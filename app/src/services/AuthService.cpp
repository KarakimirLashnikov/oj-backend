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
    {
        m_ExpireSec = std::chrono::seconds{cfg.get<int>("application", "JWT_EXPIRE", 1)};        
        m_SecretKey = cfg.get<std::string>("application", "SECRET_KEY", "");
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
        App.getRedisManager().set(info.username, data.dump());
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
            sv_info.status = NotFound;
            return sv_info;
        }
        std::string password = info.password_hash;
        auto data_str = App.getRedisManager().get(info.username);
        if (!data_str.has_value()) {
            auto db_op = makeDbOp(DbOp::OpType::Query, R"SQL(SELECT password_hash FROM users WHERE username = ?)SQL", {info.username.data()});
            njson result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get()));
            info.password_hash = result.front().at("password_hash").get<std::string>();
        } else {
            info.fromJson(njson::parse(data_str.value()));
        }

        if (!verifyPassword(password, info.password_hash)) {
            LOG_INFO("verifyPassword failed");
            sv_info.message = "password error";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        std::string token = generateUserToken(info.user_uuid).value_or("");
        if (token.empty()) {
            LOG_INFO("generateUserToken failed");
            throw SystemException{ InternalServerError, "generateToken error" };
        }

        App.getRedisManager().set(token, info.username);

        sv_info.status = OK;
        sv_info.message = "login success";
        sv_info.message["token"] = std::move(token);
        return sv_info;
    }

    bool AuthService::queryUserNameExist(std::string_view name)
    {
        LOG_INFO("search user with username: {}", name);

        auto db_op = DbOp::makeDbOp(DbOp::OpType::Query , R"SQL(SELECT * FROM users WHERE username = ?)SQL", {name.data()});
        auto result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get()));

        if (!result.empty()){
            LOG_INFO("query username {} return exist result", name);
            return true;
        }
        LOG_INFO("queryUserInfoByName return null result");
        return false;
    }

    std::optional<std::string> AuthService::generateUserToken(std::string_view user_uuid)
    {
        try
        {
            // 创建JWT构建器
            auto token_builder = jwt::create()
                                     .set_issuer("oj-system")                                            // 签发者（可选）
                                     .set_subject("user-auth")                                           // 主题（可选）
                                     .set_payload_claim("username", jwt::claim(std::string(user_uuid)))  // 用户作为载荷（payload）
                                     .set_issued_at(std::chrono::system_clock::now())                    // 签发时间
                                     .set_expires_at(std::chrono::system_clock::now() + m_ExpireSec);    // 过期时间
            // 使用HS256算法和密钥签名
            std::string token{token_builder.sign(jwt::algorithm::hs256{m_SecretKey})};
            return token;
        }
        catch (const std::exception &e)
        {
            // 捕获签名过程中的异常（如密钥为空、算法不支持等）
            LOG_ERROR("Failed to generate JWT: {}", e.what());
            return std::nullopt;
        }
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