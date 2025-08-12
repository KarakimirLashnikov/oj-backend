#pragma once
#include <sw/redis++/redis++.h>
#include "Configurator.hpp"
#include "Types.hpp"

namespace OJApp
{
    // 特定消息结构：仅支持数据库写入相关操作
    struct DbOperateMessage
    {
        enum DbOpType
        {
            INSERT, // 插入数据
            UPDATE, // 更新数据
            DELETE  // 删除数据
        };

        DbOpType op_type;       // 操作类型（必填）
        std::string table_name; // 目标表名
        std::string data_key;   // 数据唯一键（用于Redis查询）

        // 序列化：将结构转为JSON字符串（供Redis传输）
        std::string serialize() const;

        // 反序列化：从JSON字符串解析为结构（供订阅端使用）
        static DbOperateMessage deserialize(const std::string &json_str);

        // 验证消息合法性（检查必填字段）
        inline bool isValid() const
        {
            return !table_name.empty() && !data_key.empty();
        }
    };

    class RedisManager
    {
    public:
        RedisManager(Core::Configurator &cfg);
        ~RedisManager() = default;

        // 仅允许发布数据库操作消息（强类型约束）
        void publishDbOperate(const DbOperateMessage &msg);

        // 提供Redis访问接口（供订阅者从Redis获取数据）
        std::shared_ptr<sw::redis::Redis> getRedis() const { return m_Redis; }

    private:
        std::shared_ptr<sw::redis::Redis> m_Redis;

    private:
        RedisManager(const RedisManager &) = delete;
        RedisManager(RedisManager &&) = delete;
        RedisManager &operator=(RedisManager &) = delete;
        RedisManager &operator=(const RedisManager &) = delete;
    };
}
