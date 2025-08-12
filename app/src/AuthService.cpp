#include <jwt-cpp/jwt.h>
#include <sodium.h>
#include "AuthService.hpp"
#include "Application.hpp"

namespace OJApp
{
    AuthService::AuthService(Core::Configurator &cfg)
    {
        m_ExpireSec = std::chrono::seconds{cfg.get<int>("application", "JWT_EXPIRE", 1)};        
        m_SecretKey = cfg.get<std::string>("application", "SECRET_KEY", "");
    }


    bool AuthService::registryService(UserInfo info)
    {
        LOG_INFO("check info.username");
        if (queryUserNameExist(info.username)) {
            LOG_INFO("username already exist!");
            return false;
        }
        LOG_INFO("generate user pwd hash");
        auto pwd_hash{ hashPassword(info.password_hash) };
        if (!pwd_hash.has_value()) {
            LOG_ERROR("hashPassword error");
            return false;
        }
        LOG_INFO("start to registry an user (username: {})", info.username);
        info.user_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
        njson data = njson{ {"username", info.username}, {"password_hash", *pwd_hash}, {"email", info.email}, {"user_uuid", info.user_uuid} };
        DbOperateMessage msg {
            .op_type = DbOperateMessage::DbOpType::INSERT,
            .table_name = "users",
            .data_key = info.username
        };
        LOG_INFO("start to insert a new user into redis, data: {}", data.dump());
        // 缓存到redis并发布
        App.getRedisManager().getRedis()->set(msg.data_key, data.dump(), std::chrono::milliseconds{m_ExpireSec});
        App.getRedisManager().publishDbOperate(msg);
        LOG_INFO("new user {} insert into redis, and publish message to db operate channel", info.username);
        return true;
    }

    bool AuthService::queryUserNameExist(std::string_view name)
    {
        LOG_INFO("search user with username: {}", name);
        if (App.getRedisManager().getRedis()->exists(std::string(name)) == 1)
            return true;
        LOG_INFO("not exist user, try to search from db");
        auto info = App.getDBManager().queryUserInfoByName(name);
        if (info->rowsCount() != 0)
            return true;
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
                                     .set_payload_claim("user_uuid", jwt::claim(std::string(user_uuid))) // 用户ID作为载荷（payload）
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