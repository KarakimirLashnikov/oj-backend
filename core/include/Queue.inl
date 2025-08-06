#pragma once
#include "Queue.hpp"

namespace Core
{
    template <typename T>
    Queue<T>::Queue(size_t maxItems) : m_MaxSize(maxItems) {}

    template <typename T>
    bool Queue<T>::push(T&& item) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_NotFullCV.wait(lock, [this]() { 
            return m_Queue.size() < m_MaxSize || m_Shutdown; 
        });
        if (m_Shutdown) 
            return false;
        m_Queue.push(std::move(item));
        m_NotEmptyCV.notify_one();
        return true;
    }

    template <typename T>
    bool Queue<T>::pop(T& item, Types::Millis timeout) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        if (!m_NotEmptyCV.wait_for(lock, timeout, [this]() { return !m_Queue.empty() || m_Shutdown; })) {
            return false; // Timeout
        }
        if (m_Shutdown && m_Queue.empty()) return false;
        item = std::move(m_Queue.front());
        m_Queue.pop();
        m_NotFullCV.notify_one();
        return true;
    }

    template <typename T>
    void Queue<T>::shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Shutdown = true;
        }
        m_NotEmptyCV.notify_all();
        m_NotFullCV.notify_all();
    }
}