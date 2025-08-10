#pragma once
#include "User.hpp"
#include "DBManager.hpp"
#include "RedisCache.hpp"

namespace OJApp
{
    class AuthService
    {

    public:
        AuthService(Database::DBManager* db_manager,
                    Database::RedisCache* redis_cache,
                    const std::string &secret_key,
                    std::chrono::seconds token_expiry);
        ~AuthService() = default;

        // 注册新用户
        bool registerUser(const std::string &username, const std::string &email,
                           const std::string &password, std::string &error_message);

        // 用户登录并获取token
        bool loginUser(const std::string &username, const std::string &password, const std::string &email,
                        std::string &token, std::string &error_message);

        // 验证token并获取用户ID
        bool validateToken(const std::string &token, std::string &user_id);

        // 登出使token失效
        bool logoutUser(const std::string &token);

        // 获取当前登录用户信息
        std::unique_ptr<User> getCurrentUser(const std::string &token);

    private:
        // 密码哈希相关
        std::string generateSalt();
        std::string hashPassword(const std::string &password, const std::string &salt);
        bool verifyPassword(const std::string &password, const std::string &salt,
                             const std::string &stored_hash);

        // Token相关
        std::string generateToken(const std::string &user_id);
        bool verifyToken(const std::string &token, std::string &user_id);

    private:
        Database::DBManager* m_DBManagerPtr;
        Database::RedisCache* m_RedisCachePtr;
        std::string m_SecretKey;
        std::chrono::seconds m_TokenExpiry;
    };
}