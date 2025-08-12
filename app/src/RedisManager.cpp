#include "RedisManager.hpp"
#include "Logger.hpp"
#include "Core.hpp"
#include "SystemException.hpp"
#include "ParameterException.hpp"

static std::string DB_OPERATE_CHANNEL{"db_operate_channel"};

namespace OJApp
{
    using Exceptions::makeSystemException;
    using Exceptions::ParameterException;

    std::string DbOperateMessage::serialize() const
    {
        njson j{};
        j["op_type"] = static_cast<int>(op_type);
        j["table_name"] = table_name;
        j["data_key"] = data_key;
        return j.dump();
    }

    DbOperateMessage DbOperateMessage::deserialize(const std::string &json_str)
    {
        auto j = nlohmann::json::parse(json_str);
        DbOperateMessage msg;
        msg.op_type = static_cast<DbOpType>(j["op_type"].get<int>());
        msg.table_name = j["table_name"].get<std::string>();
        msg.data_key = j["data_key"].get<std::string>();
        return msg;
    }

    RedisManager::RedisManager(Core::Configurator &cfg)
    {
        ::DB_OPERATE_CHANNEL = cfg.get<std::string>("redis", "OPERATE_CHANNEL", "db_operate_channel");

        sw::redis::ConnectionOptions opts{};
        opts.host = cfg.get<std::string>("redis", "HOST", "127.0.0.1");
        opts.port = cfg.get<uint16_t>("redis", "PORT", 6379);
        opts.password = cfg.get<std::string>("redis", "PASSWORD", "");
        opts.db = cfg.get<size_t>("redis", "DB_INDEX", 0);
        opts.connect_timeout = std::chrono::milliseconds(cfg.get<int>("redis", "CONNECT_TIMEOUT", 5000));
        opts.socket_timeout = std::chrono::milliseconds(cfg.get<int>("redis", "SOCKET_TIMEOUT", 10000));
        opts.keep_alive = cfg.get<bool>("redis", "KEEP_ALIVE", true);

        {
            sw::redis::Redis testConn(opts);
            if (testConn.ping() != "PONG") {
                LOG_ERROR("Redis connection test failed");
                throw makeSystemException("Redis connection test failed");
            }
            LOG_INFO("Redis connection test OK");
        }

        sw::redis::ConnectionPoolOptions connPoolOpts{};
        connPoolOpts.size = cfg.get<size_t>("redis", "POOL_SIZE", 4);

        m_Redis = std::make_shared<sw::redis::Redis>(opts, connPoolOpts);
    }

    // 发布数据库操作消息（仅接受规范结构）
    void RedisManager::publishDbOperate(const DbOperateMessage& msg) {
        // 1. 验证消息合法性（必填字段检查）
        if (!msg.isValid()) {
            LOG_ERROR("Invalid DbOperateMessage: table_name or data_key is empty");
            throw makeSystemException("Invalid message structure");
        }

        try {
            // 2. 内部序列化（用户无法直接修改传输内容）
            std::string serialized = msg.serialize();

            // 3. 发布到固定频道（用户无需指定频道，防止频道混乱）
            m_Redis->publish(DB_OPERATE_CHANNEL, serialized);
            LOG_INFO("Published db operate: {}:{}:{}", msg.table_name, msg.data_key, static_cast<int>(msg.op_type));
        } catch (const sw::redis::Error& e) {
            LOG_ERROR("Publish failed: {}", e.what());
            throw makeSystemException("Redis publish error");
        }
    }
}
