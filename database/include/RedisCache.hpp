#pragma once
#include <sw/redis++/redis++.h>
#include "Configurator.hpp"

namespace Database
{
    class RedisCache
    {
    public:
        RedisCache() = default;
        ~RedisCache() = default;

        bool init(Core::Configurator& cfg);

        // Token 相关操作
        bool setToken(const std::string &token, const std::string &user_id, std::chrono::seconds expiry);
        std::string getUserIdFromToken(const std::string &token);
        bool invalidateToken(const std::string &token);
        bool isTokenValid(const std::string &token);

    private:
        std::shared_ptr<sw::redis::Redis> m_Redis;
    };
}