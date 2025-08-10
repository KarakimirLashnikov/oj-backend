#include "RedisCache.hpp"
#include "Logger.hpp"
#include "Core.hpp"

namespace Database
{
    bool RedisCache::init(Core::Configurator &cfg)
    {
        try
        {
            // 安全获取配置项，提供默认值避免异常
            sw::redis::ConnectionOptions opts;
            opts.host = cfg.get<std::string>("redis", "HOST", "127.0.0.1");
            opts.port = cfg.get<uint16_t>("redis", "PORT", 6379);
            opts.password = cfg.get<std::string>("redis", "PASSWORD", "");
            opts.db = cfg.get<size_t>("redis", "DB_INDEX", 0);
            opts.connect_timeout = std::chrono::milliseconds(
                cfg.get<int>("redis", "CONNECT_TIMEOUT", 500));
            opts.socket_timeout = std::chrono::milliseconds(
                cfg.get<int>("redis", "SOCKET_TIMEOUT", 10000));

            // 使用临时对象测试连接
            {
                sw::redis::Redis testConn(opts);
                if (testConn.ping() != "PONG")
                {
                    LOG_ERROR("Redis connection test failed");
                    return false;
                }
            }

            // 测试通过后创建持久连接
            m_Redis = std::make_shared<sw::redis::Redis>(opts);

            return true;
        }
        // 捕获所有可能的异常类型
        catch (const std::exception &ex)
        {
            LOG_ERROR("Redis initialization failed: {}", ex.what());
        }
        catch (...)
        {
            LOG_ERROR("Unknown error during Redis initialization");
        }

        return false;
    }
    bool RedisCache::setToken(const std::string &token, const std::string &user_id, std::chrono::seconds expiry)
    {
        try
        {
            m_Redis->set(token, user_id, expiry);
            m_Redis->expire(token, expiry);
        }
        catch (const sw::redis::Error &err)
        {
            LOG_ERROR("Redis setToken failed: {}", err.what());
            return false;
        }
        return true;
    }

    std::string RedisCache::getUserIdFromToken(const std::string &token)
    {
        try
        {
            auto val{m_Redis->get(token)};
            if (val)
                return *val;
        }
        catch (const sw::redis::Error &err)
        {
            LOG_ERROR("Redis getToken failed: {}", err.what());
        }
        return {};
    }
    bool RedisCache::invalidateToken(const std::string &token)
    {
        try
        {
            m_Redis->del(token);
        }
        catch (const sw::redis::Error &err)
        {
            LOG_ERROR("Redis invalidateToken failed: {}", err.what());
            return false;
        }
        return true;
    }
    bool RedisCache::isTokenValid(const std::string &token)
    {
        try
        {
            return m_Redis->exists(token);
        }
        catch (const sw::redis::Error &err)
        {
            LOG_ERROR("Redis isTokenValid failed: {}", err.what());
        }
        return false;
    }
}
