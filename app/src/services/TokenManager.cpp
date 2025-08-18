#include <jwt-cpp/jwt.h>
#include "services/TokenManager.hpp"
#include "Application.hpp"

using Core::Types::UserInfo;

OJApp::TokenManager::TokenManager(Core::Configurator &cfg)
: m_JWTSecretKey{ cfg.get<std::string>("application", "JWT_SECRET_KEY") }
, m_ExpireTime{ std::chrono::seconds{cfg.get<int>("application", "JWT_EXPIRE", 1800)} }
, m_ActivePrefix{ cfg.get<std::string>("application", "REDIS_ACTIVE_PREFIX", "user:active:") }
, m_BlacklistPrefix{ cfg.get<std::string>("application", "REDIS_BLACKLIST_PREFIX", "user:token_blacklist:") }
{
}

std::optional<std::string> OJApp::TokenManager::generateToken(const UserInfo &user)
{
    try {
        auto token{
            jwt::create()
            .set_issuer("oj-system")
            .set_subject(user.user_uuid)
            .set_payload_claim("username", jwt::claim(user.username))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + m_ExpireTime)
            .sign(jwt::algorithm::hs256{m_JWTSecretKey})
        };

        std::string redis_key{ m_ActivePrefix + user.user_uuid };
        if (!App.getRedisManager().set(redis_key, token, m_ExpireTime)) {
            LOG_ERROR("generate token set redis failed");
            return std::nullopt;
        }

        return token;
    } catch (const std::exception &e) {
        LOG_ERROR("generate token error: {}", e.what());
        return std::nullopt;
    }
}

std::optional<UserInfo> OJApp::TokenManager::verifyToken(const std::string &token)
{
    try {
        auto decoded{ jwt::decode(token) };
        auto verifier{
            jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{m_JWTSecretKey})
            .with_issuer("oj-system")
        };

        verifier.verify(decoded);

        std::string blacklist_key{ m_BlacklistPrefix + token };
        if (App.getRedisManager().exists(blacklist_key)) {
            LOG_INFO("token blacklisted");
            return std::nullopt;
        }

        auto user_id{ decoded.get_subject() };
        std::string active_key{ m_ActivePrefix + user_id };
        auto active_token{ App.getRedisManager().get(active_key) };

        if (!active_token || active_token != token) {
            LOG_INFO("token no longer valid");
            return std::nullopt;
        }

        UserInfo user{};
        user.user_uuid = user_id;
        user.username = decoded.get_payload_claim("username").as_string();
        return user;
    } catch (const std::exception &e) {
        LOG_ERROR("verify token error: {}", e.what());
        return std::nullopt;
    }
}

bool OJApp::TokenManager::invalidateToken(const std::string &token)
{
    try {
        auto expiry{ getTokenExpiry(token) };
        if (!expiry) {
            LOG_INFO("token does not exist");
            return false;
        }

        auto now{ std::chrono::system_clock::now() };
        if (*expiry <= now) {
            LOG_INFO("token has expired");
            return true;
        }

        auto ttl{ duration_cast<std::chrono::seconds>(*expiry - now) };

        std::string blacklist_key{ m_BlacklistPrefix + token };
        if (!App.getRedisManager().set(blacklist_key, "1", ttl)) {
            LOG_ERROR("blacklist token set redis failed");
            return false;
        }

        auto decoded{ jwt::decode(token) };
        std::string active_key{ m_ActivePrefix + decoded.get_subject() };
        if (!App.getRedisManager().del(active_key)) {
            LOG_ERROR("remove token from active set redis failed");
            return false;
        }

        return true;
    } catch (const std::exception &e) {
        LOG_ERROR("invalidate token error: {}", e.what());
        return false;
    }
}

std::optional<std::string> OJApp::TokenManager::refreshToken(const std::string &token)
{
    try {
        auto user{ verifyToken(token) };
        if (!user) {
            LOG_INFO("token verify failed");
            return std::nullopt;
        }

        if (!invalidateToken(token)) {
            LOG_INFO("invalidate token failed");
            return std::nullopt;
        }

        return generateToken(*user);
    } catch (const std::exception &e) {
        LOG_ERROR("refresh token error: {}", e.what());
        return std::nullopt;
    }
}