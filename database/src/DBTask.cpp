#include "DBTask.hpp"

namespace Database
{
    DBTask::DBTask(std::coroutine_handle<promise_type> handler) noexcept
        : m_Handler{ handler }
    {
    }
    DBTask::~DBTask() noexcept
    {
        if (m_Handler) {
            m_Handler.destroy();
        }
    }
    DBTask::DBTask(DBTask &&other) noexcept
        : m_Handler{ std::exchange(other.m_Handler, nullptr) }
    {
    }
    DBTask &DBTask::operator=(DBTask &&other) noexcept
    {
        if (this != &other) {
            if (m_Handler)
                m_Handler.destroy();
            m_Handler = std::exchange(other.m_Handler, nullptr);
        }
        return *this;
    }
    void DBTask::resume() noexcept
    {
        m_Handler.resume();
    }
}