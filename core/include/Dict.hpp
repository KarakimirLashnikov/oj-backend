#pragma once
#include "Core.hpp"

namespace Core
{
    template <typename Key, typename Value>
    class Dict
    {
        std::map<Key, Value> m_Data;
        // 使用shared_mutex允许多个读者同时访问，写操作独占
        mutable std::shared_mutex m_Mutex;

    public:
        // 默认构造函数
        Dict() = default;

        // 禁止拷贝构造和赋值，避免线程安全问题
        Dict(const Dict &) = delete;
        Dict &operator=(const Dict &) = delete;

        // 允许移动构造和赋值
        Dict(Dict &&other) noexcept;

        Dict &operator=(Dict &&other) noexcept;

        // 插入或更新元素
        void insert(const Key &key, const Value &value);

        // 查找元素，返回optional<Value>
        // 如果存在则返回对应值，否则返回nullopt
        std::optional<Value> find(const Key &key) const;

        // 删除元素，返回是否成功删除
        bool erase(const Key &key);

        // 获取当前元素数量
        size_t size() const;

        // 清空所有元素
        void clear();
    };
}

#include "Dict.inl"