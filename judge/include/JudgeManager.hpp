#pragma once

#include "Core.hpp"
#include "ThreadPool.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Submission.hpp"

namespace Judge
{
    class JudgeManager
    {
    public:
        JudgeManager(std::function<void(JudgeResult &&result)> &&callback,
                     size_t worker_num = std::thread::hardware_concurrency());
        ~JudgeManager() = default;

        void submit(Submission &&submission);

    private:
        std::unique_ptr<Core::ThreadPool> m_ThreadPool;
        const std::function<void(JudgeResult &&result)> m_JudgeResultCallback;
    };

    inline std::unique_ptr<JudgeManager>
    createManager(std::function<void(JudgeResult &&result)> &&callback,
                  size_t worker_num = std::thread::hardware_concurrency()) {
        return std::make_unique<JudgeManager>(std::move(callback), worker_num);
    }
}