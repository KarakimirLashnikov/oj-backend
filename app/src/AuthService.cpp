#include <openssl/rand.h>
#include <openssl/sha.h>
#include <jwt-cpp/jwt.h>
#include "AuthService.hpp"
#include "Logger.hpp"
#include "Core.hpp"
#include "Types.hpp"
#include "Application.hpp"

namespace OJApp
{
    // 构造函数实现
    AuthService::AuthService(std::shared_ptr<Database::DBManager> db_manager,
                            std::shared_ptr<Database::RedisCache> redis_cache,
                            const std::string &secret_key,
                            std::chrono::seconds token_expiry)
        : m_DBManagerPtr{ db_manager }
        , m_RedisCachePtr{ redis_cache }
        , m_SecretKey{ secret_key }
        , m_TokenExpiry{ token_expiry }
    {
    }

    // 注册用户实现
    bool AuthService::registerUser(const std::string &username,
                                   const std::string &email,
                                   const std::string &password,
                                   std::string &error_message)
    {
        try
        {
            // 1. 验证用户名是否已存在
            if (m_DBManagerPtr->isUsernameExist(username)) {
                error_message = "Username already exists";
                return false;
            }

            // 2. 验证邮箱是否已存在
            if (m_DBManagerPtr->isEmailExist(email)) {
                error_message = "Email already exists";
                return false;
            }

            // 3. 密码处理
            const std::string salt = generateSalt();
            const std::string password_hash = hashPassword(password, salt);

            // 4. 准备用户数据
            UserInfo user_info{
                .user_uuid = boost::uuids::to_string(boost::uuids::random_generator()()),
                .username = username,
                .password_hash = password_hash,
                .email = email,
                .salt = salt
            };
            
            App.addDBTask(m_DBManagerPtr->insertUser(user_info));

            LOG_INFO("User registered successfully: {}", username);
            return true;
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Database error during registration: {}", e.what());
            error_message = "Registration failed: database error";
            return false;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Error during registration: {}", e.what());
            error_message = "Registration failed: internal error";
            return false;
        }
    }

    // 用户登录实现
    bool AuthService::loginUser(const std::string &username,
                                const std::string &password,
                                std::string &token,
                                std::string &error_message)
    {
        try
        {
            // 1. 查询用户
            LOG_INFO("Getting user by username: {}", username);
            UserInfo user{ m_DBManagerPtr->queryUserInfoByName(username) };
            LOG_INFO("User retrieved: {}\n{}\n{}\n{}", user.username, user.user_uuid, user.salt, user.password_hash);
            // 2. 验证密码
            if (!verifyPassword(password, user.salt, user.password_hash)) {
                error_message = "Password is incorrect";
                return false;
            }
            LOG_INFO("Password is correct for user: {}", username);
            // 4. 生成并存储token
            token = generateToken(user.user_uuid);
            m_RedisCachePtr->setToken(token, username, m_TokenExpiry);
            LOG_INFO("User logged in: {}", username);
            return true;
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Database error during login: {}", e.what());
            error_message = "Login failed: database error";
            return false;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Error during login: {}", e.what());
            error_message = "Login failed: internal error";
            return false;
        }
    }

    // 验证token实现
    bool AuthService::validateToken(const std::string &token, std::string &user_id)
    {
        try
        {
            // 1. 先验证JWT签名和有效期
            if (!verifyToken(token, user_id))
                return false;

            // 2. 再检查Redis中是否有效
            if (!m_RedisCachePtr->isTokenValid(token))
                return false;

            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Token validation error: {}", e.what());
            return false;
        }
    }

    // 登出实现
    bool AuthService::logoutUser(const std::string &token)
    {
        try
        {
            m_RedisCachePtr->invalidateToken(token);
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Logout error: {}", e.what());
            return false;
        }
    }

    // 获取当前用户实现
    std::optional<UserInfo> AuthService::getCurrentUser(const std::string &token)
    {
        try
        {
            std::string user_uuid{};
            if (!validateToken(token, user_uuid)) {
                return std::nullopt;
            }

            UserInfo user_info{ m_DBManagerPtr->queryUserInfoByUUID(user_uuid) };
            return user_info;
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR("Database error in getCurrentUser: {}", e.what());
            return std::nullopt;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Error in getCurrentUser: {}", e.what());
            return std::nullopt;
        }
    }

    // 生成盐值
    std::string AuthService::generateSalt()
    {
        unsigned char salt[16];
        if (RAND_bytes(salt, sizeof(salt)) != 1)
        {
            throw std::runtime_error("Failed to generate cryptographically secure salt");
        }

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (unsigned char b : salt)
        {
            ss << std::setw(2) << static_cast<int>(b);
        }
        return ss.str();
    }

    // 密码哈希 使用SHA256 + 盐值
    std::string AuthService::hashPassword(const std::string &password, const std::string &salt)
    {
        const std::string input = password + salt;
        unsigned char hash[SHA256_DIGEST_LENGTH];

        SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (unsigned char b : hash)
        {
            ss << std::setw(2) << static_cast<int>(b);
        }
        LOG_INFO("SHA256 hash generated: {}", ss.str());
        return ss.str();
    }

    // 密码验证
    bool AuthService::verifyPassword(std::string password,
                                     std::string salt,
                                     std::string stored_hash)
    {
        return hashPassword(password, salt) == stored_hash;
    }

    // 生成JWT Token
    std::string AuthService::generateToken(const std::string &user_id)
    {
        const auto now = std::chrono::system_clock::now();
        const auto expiry = now + m_TokenExpiry;

        return jwt::create()
            .set_issuer("OJApp")
            .set_subject(user_id)
            .set_issued_at(now)
            .set_expires_at(expiry)
            .sign(jwt::algorithm::hs256{m_SecretKey});
    }

    // 验证JWT Token
    bool AuthService::verifyToken(const std::string &token, std::string &user_id)
    {
        try
        {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                                .allow_algorithm(jwt::algorithm::hs256{m_SecretKey})
                                .with_issuer("OJApp");

            verifier.verify(decoded);
            user_id = decoded.get_subject();
            return true;
        }
        catch (const jwt::error::token_verification_exception &e)
        {
            LOG_WARN("Token verification failed: {}", e.what());
            return false;
        }
    }

}
