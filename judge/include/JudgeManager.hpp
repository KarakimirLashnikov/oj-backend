#pragma once

#include "Core.hpp"
#include "SubmissionSchema.hpp"
#include "JudgeResult.hpp"
#include "ThreadPool.hpp"

namespace Judge
{
    class JudgeManager
    {
    public:
        JudgeManager(std::string_view url, 
            std::string_view auth_token,
            std::function<void(SubmissionResult&&)> callback,
            int max_workers = std::thread::hardware_concurrency());

        bool submit(SubmissionSchema&& schema);

    private:
        void insertTaskResult(std::uint64_t id, TaskResult&& task_result);

        std::string m_JudgeUrl;
        std::string m_AuthToken;
        std::unique_ptr<Core::ThreadPool> m_Workers;
        std::mutex m_TokenWriteMutex;
        std::unordered_map<std::string, std::pair<std::uint64_t, int>> m_Token2Id_Index;
        std::unordered_map<std::uint64_t, SubmissionResult> m_SubmissionId2Result;
        std::function<void(SubmissionResult&&)> m_Callback;
    };
}