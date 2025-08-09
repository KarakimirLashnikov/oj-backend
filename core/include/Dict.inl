#include "Dict.hpp"

namespace Core
{
    template <typename Key, typename Value>
    Dict<Key, Value>::Dict(Dict &&other) noexcept
    {
        std::lock_guard<std::shared_mutex> lock(other.m_Mutex);
        m_Data = std::move(other.m_Data);
    }

    template <typename Key, typename Value>
    Dict<Key, Value> &Dict<Key, Value>::operator=(Dict &&other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::shared_mutex> lock1(m_Mutex);
            std::lock_guard<std::shared_mutex> lock2(other.m_Mutex);
            m_Data = std::move(other.m_Data);
        }
        return *this;
    }

    template <typename Key, typename Value>
    void Dict<Key, Value>::insert(const Key &key, const Value &value)
    {
        // 写操作使用独占锁
        std::lock_guard<std::shared_mutex> lock(m_Mutex);
        m_Data[key] = value;
    }

    template <typename Key, typename Value>
    std::optional<Value> Dict<Key, Value>::find(const Key &key) const
    {
        // 读操作使用共享锁，允许多个读者同时访问
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        auto it = m_Data.find(key);
        if (it != m_Data.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    template <typename Key, typename Value>
    bool Dict<Key, Value>::erase(const Key &key)
    {
        std::lock_guard<std::shared_mutex> lock(m_Mutex);
        return m_Data.erase(key) > 0;
    }

    template <typename Key, typename Value>
    size_t Dict<Key, Value>::size() const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        return m_Data.size();
    }

    template <typename Key, typename Value>
    void Dict<Key, Value>::clear()
    {
        std::lock_guard<std::shared_mutex> lock(m_Mutex);
        m_Data.clear();
    }
}