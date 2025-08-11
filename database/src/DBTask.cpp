#include "DBTask.hpp"
#include "Logger.hpp"

namespace Database
{
    DBTask::DBTask(std::coroutine_handle<promise_type> handler) noexcept
        : m_Handler{ handler }
    {
    }
    DBTask::~DBTask() noexcept
    {
        if (m_Handler) {
            if (m_Handler.promise().eptr) {
                std::rethrow_exception(m_Handler.promise().eptr);
            }
            m_Handler.destroy();
            std::cout << "delete db task" << std::endl;
            LOG_INFO("DBTask has been destroyed.");
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