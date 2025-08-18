#pragma once
#include <sw/redis++/redis++.h>
#include "Configurator.hpp"
#include "Types.hpp"
#include "dbop/DbOperation.hpp"

namespace OJApp
{
    // 特定消息结构：仅支持数据库写入相关操作
    struct DbOperateMessage
    {
        using DbOpType = DbOp::OpType;
        DbOpType op_type;       // 操作类型（必填）
        std::string sql;        // SQL语句（必填）
        std::string data_key;   // 数据唯一键（用于Redis查询）
        njson param_array;      // 参数

        // 序列化：将结构转为JSON字符串（供Redis传输）
        std::string serialize() const;

        // 反序列化：从JSON字符串解析为结构（供订阅端使用）
        static DbOperateMessage deserialize(const std::string &json_str);

        // 验证消息合法性（检查必填字段）
        inline bool isValid() const
        {
            return op_type != DbOpType::INVALID_OP && !sql.empty() && !data_key.empty();
        }
    };

    class RedisManager
    {
    public:
        RedisManager(Core::Configurator &cfg);
        ~RedisManager() = default;

        // 仅允许发布数据库操作消息（强类型约束）
        void publishDbOperate(DbOperateMessage msg);

        // 提供Redis访问接口（供订阅者从Redis获取数据）
        std::shared_ptr<sw::redis::Redis> getRedis() const { return m_Redis; }

        bool set(const std::string &key, const std::string &value);

        bool set(const std::string &key, const std::string &value, const std::chrono::seconds &ttl);

        bool del(const std::string &key);

        bool exists(const std::string& key);

        bool expire(const std::string& key);

        std::optional<std::string> get(const std::string& key);

    private:
        std::shared_ptr<sw::redis::Redis> m_Redis;
        std::chrono::seconds m_KeyTTL;

    private:
        RedisManager(const RedisManager &) = delete;
        RedisManager(RedisManager &&) = delete;
        RedisManager &operator=(RedisManager &) = delete;
        RedisManager &operator=(const RedisManager &) = delete;
    };
}
