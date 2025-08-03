#include "JudgeManager.hpp"
#include "Judger.hpp"

namespace Judge
{
    JudgeManager::JudgeManager(JudgeCallbackFn &&callback, size_t worker_num)
    : m_JudgeResultCallback{ std::forward<JudgeCallbackFn>(callback) }
    , m_ThreadPool{ std::make_unique<Core::ThreadPool>(worker_num) }
    {
    }

    void JudgeManager::submit(Submission submission)
    {
        LOG_INFO("JudgeManager submitted: {}", __FILE__);
        m_JudgeResultCallback(
            submission.submission_id, this->m_ThreadPool->enqueue(
                [submission]() mutable -> JudgeResult {
                    // 从数据库读取测试数据集以及限制时间、空间限制等信息，构造提交对象
                    // TODO: 
                    {
                        submission.cpu_extra_time_s = 0.5;
                        submission.cpu_time_limit_s = 1.0;
                        submission.memory_limit_kb = 64000;
                        submission.stack_limit_kb = 32000;
                        submission.wall_time_limit_s = 3.0;
                        submission.test_cases.insert(submission.test_cases.end(), 
                            { { "0 0", "0"}
                            , { "1 1", "2"}
                            , { "3 3", "6"}
                            , { "6 6", "12"}
                            , { "2 4", "6"}
                            , { "6 12", "18"}}
                        );
                    }
                    Judger judger{};
                    return judger.judge(submission);
                }
            )
        );
    }
}