#pragma once
#include "Core.hpp"
#include "Types.hpp"

namespace Core
{
    template <typename T>
    class Queue
    {
    public:
        Queue() = default;
        Queue(size_t maxItems);

        bool push(T &&item);
        bool pop(T &item, Types::Millis timeout);
        void shutdown();

    private:
        std::queue<T> m_Queue;
        mutable std::mutex m_Mutex;
        std::condition_variable m_NotEmptyCV;
        std::condition_variable m_NotFullCV;
        size_t m_MaxSize{128};
        bool m_Shutdown{false};
    };
}


#include "Queue.inl"