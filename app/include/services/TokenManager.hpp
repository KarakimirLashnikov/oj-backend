#pragma once

#include "Types.hpp"
#include "Configurator.hpp"

namespace OJApp
{
    using Core::Types::UserInfo;

    class TokenManager
    {
    public:
        TokenManager(Core::Configurator& cfg);

        ~TokenManager() = default;

        std::optional<std::string> generateToken(const UserInfo& user);

        std::optional<UserInfo> verifyToken(const std::string& token);

        bool invalidateToken(const std::string& token);

        std::optional<std::string> refreshToken(const std::string& token);

        inline std::string getActiveRedisKey() { return m_ActivePrefix; }
        inline std::string getBlacklistRedisKey() { return m_BlacklistPrefix; }
    private:
        std::optional<std::chrono::system_clock::time_point> getTokenExpiry(const std::string& token);

    private:
        std::string m_JWTSecretKey;
        std::string m_ActivePrefix;
        std::string m_BlacklistPrefix;
        std::chrono::seconds m_ExpireTime;
    };
}