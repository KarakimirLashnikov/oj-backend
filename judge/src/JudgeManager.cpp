#include "JudgeManager.hpp"
#include "Judger.hpp"

namespace Judge
{
    JudgeManager::JudgeManager(std::function<void(JudgeResult &&result)> &&callback, size_t worker_num)
    : m_JudgeResultCallback{ std::forward<std::function<void(JudgeResult &&result)>>(callback) }
    , m_ThreadPool{ std::make_unique<Core::ThreadPool>(worker_num) }
    {
    }

    void JudgeManager::submit(Submission &&submission)
    {
        auto future_result{ m_ThreadPool->enqueue([&]() -> JudgeResult {
                    Judger judger{};
                    return judger.judge(std::move(submission));
                }
            )
        };

        m_JudgeResultCallback(std::move(future_result.get()));
    }
}