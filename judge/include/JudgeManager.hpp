#pragma once

#include "Core.hpp"
#include "Types.hpp"
#include "ThreadPool.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Submission.hpp"

namespace Judge
{
    using JudgeCallbackFn = std::function<void(SubID, std::future<Judge::JudgeResult>&&)>;
    class JudgeManager
    {
    public:
        JudgeManager(JudgeCallbackFn &&callback,
                     size_t worker_num = std::thread::hardware_concurrency());
        ~JudgeManager() = default;

        void submit(Submission submission);

    private:
        std::unique_ptr<Core::ThreadPool> m_ThreadPool;
        JudgeCallbackFn m_JudgeResultCallback;
    };

    inline std::unique_ptr<JudgeManager>
    createManager(JudgeCallbackFn &&callback, size_t worker_num = std::thread::hardware_concurrency()) {
        return std::make_unique<JudgeManager>(std::move(callback), worker_num);
    }
}