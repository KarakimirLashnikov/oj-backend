#include <jwt-cpp/jwt.h>
#include <sodium.h>
#include "services/AuthService.hpp"
#include "Application.hpp"
#include "SystemException.hpp"
#include "dbop/DbOpFactory.hpp"


namespace OJApp
{
    using Exceptions::SystemException;
    
    AuthService::AuthService(Core::Configurator &cfg)
    : IAuthRequired{ cfg }
    {
    }


    ServiceInfo AuthService::registryService(UserInfo info)
    {
        ServiceInfo sv_info{};
        if (queryUserNameExist(info.username)) {
            LOG_INFO("user exist, return username exist response");
            sv_info.message["message"] = "username exist, please try other name";
            sv_info.status = Conflict;
            return sv_info;
        }

        auto pwd_hash{ hashPassword(info.password_hash) };
        if (!pwd_hash.has_value()) {
            LOG_INFO("hashPassword failed");
            throw SystemException{ InternalServerError, "hashPassword error" };
        }

        info.password_hash = *pwd_hash;
        info.user_uuid = boost::uuids::to_string(boost::uuids::random_generator()());

        njson data = info.toJson();
        DbOperateMessage msg {
            .op_type = DbOperateMessage::DbOpType::Insert,
            .sql = R"SQL(INSERT INTO users (user_uuid,username,password_hash,email) VALUES(?,?,?,?))SQL",
            .data_key = info.username,
            .param_array = {data["user_uuid"], data["username"], data["password_hash"], data["email"]}
        };
        // 缓存到redis并发布
        App.getRedisManager().set(info.username, info.toJsonString(), std::chrono::seconds{900});
        App.getRedisManager().publishDbOperate(msg);

        sv_info.message["message"] = "registration success";
        sv_info.status = Created;
        return sv_info;
    }

    using DbOp::makeDbOp;
    ServiceInfo AuthService::loginService(UserInfo info)
    {
        ServiceInfo sv_info{};
        LOG_INFO("start to query user {} from db", info.username);
        if (!queryUserNameExist(info.username)) {
            LOG_INFO("user does not exist");
            sv_info.message["message"] = "username does not exist, please register firstly";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        std::string password = info.password_hash;
        auto data_str = App.getRedisManager().get(info.username);
        if (!data_str) {
            auto db_op = makeDbOp(DbOp::OpType::Query, R"SQL(SELECT * FROM users WHERE username = ? LIMIT 1)SQL", {info.username.data()});
            njson result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get())).front();
            info.fromJson(result);
        } else {
            info.fromJson(njson::parse(data_str.value()));
        }

        if (!verifyPassword(password, info.password_hash)) {
            LOG_INFO("verifyPassword failed");
            sv_info.message["message"] = "password error";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        std::string token{ m_TokenManager->generateToken(info).value_or(std::string{}) };
        if (token.empty()) {
            LOG_INFO("generateUserToken failed");
            throw SystemException{ InternalServerError, "generateToken error" };
        }

        sv_info.status = OK;
        sv_info.message["message"] = "login success";
        sv_info.message["token"] = std::move(token);
        return sv_info;
    }

    bool AuthService::queryUserNameExist(std::string_view name)
    {
        LOG_INFO("search user with username: {}", name);
        if (App.getRedisManager().exists(name.data())) {
            LOG_INFO("username already exists");
            return false;
        }
        auto db_op = DbOp::makeDbOp(DbOp::OpType::Query , R"SQL(SELECT * FROM users WHERE username = ?)SQL", {name.data()});
        auto result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get()));
        if (!result.empty()){
            LOG_INFO("username already exists");
            return true;
        }
        LOG_INFO("username doesn't exist");
        return false;
    }

    std::optional<std::string> AuthService::hashPassword(std::string_view pwd)
    {
        char hash_buf[crypto_pwhash_STRBYTES];
        
        // 生成哈希（使用默认参数）
        // crypto_pwhash_OPSLIMIT_INTERACTIVE：适合交互式场景（如登录）
        // crypto_pwhash_MEMLIMIT_INTERACTIVE：对应内存限制
        int result = crypto_pwhash_str(
            hash_buf,
            pwd.data(),
            pwd.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE
        );

        if (result != 0) {
            LOG_ERROR("hashPassword error: crypto_pwhash_str return code = {}", result);
            return std::nullopt;
        }

        return std::string(hash_buf);
    }

     bool AuthService::verifyPassword(std::string_view password, std::string_view hash) 
     {
        int result = crypto_pwhash_str_verify(
            hash.data(),
            password.data(),
            password.size()
        );
        return result == 0;
    }
}