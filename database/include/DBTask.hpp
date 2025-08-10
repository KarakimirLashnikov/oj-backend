#pragma once
#include "Core.hpp"

namespace Database
{
    struct DBTask
    {
        struct promise_type
        {
            std::exception_ptr eptr;

            inline DBTask get_return_object() {
                return DBTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }

            inline std::suspend_always initial_suspend() { return {}; }
            inline std::suspend_always final_suspend() noexcept { return {}; }
            inline void unhandled_exception() { eptr = std::current_exception(); }
            inline void return_void() {}
        };

        DBTask() = default;
        DBTask(std::coroutine_handle<promise_type> handler) noexcept;
        ~DBTask() noexcept;
        DBTask(DBTask&& other) noexcept;
        DBTask& operator=(DBTask&& other) noexcept;

        void resume() noexcept;

    private:
        std::coroutine_handle<promise_type> m_Handler{ nullptr };

        DBTask(const DBTask&) = delete;
        DBTask& operator=(const DBTask&) = delete;
    };
}