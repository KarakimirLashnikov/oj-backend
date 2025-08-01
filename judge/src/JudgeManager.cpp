#include "JudgeManager.hpp"
#include "Judger.hpp"

namespace Judge
{
    JudgeManager::JudgeManager(std::string_view url,
                               std::string_view auth_token,
                               int max_workers)
        : m_JudgeUrl{ url.data() }
        , m_AuthToken{ auth_token.data() }
        , m_Workers{ std::make_unique<Core::ThreadPool>(static_cast<std::size_t>(max_workers)) }
    {
    }

    bool JudgeManager::submit(SubmissionSchema &&schema)
    {
        {
            std::lock_guard lock{ this->m_ResultWriteMutex };
            this->m_SubmissionId2Result.insert(
                { std::make_pair(schema.submission_id, SubmissionResult(schema.submission_id, schema.test_cases.size()))}
            );
        }
        try {
            this->m_Workers->enqueue([this, schema = std::move(schema)]() -> void {
                auto params{ SubmissionSchema::toHttpParams(schema) };
                Judger judger{ this->m_JudgeUrl, this->m_AuthToken, 3, 10 };
                std::vector<std::future<std::string>> tokens{};
                std::for_each(params.begin(), params.end(), [&judger, &tokens](httplib::Params &param) { 
                    auto token{ std::async(std::launch::async, [&judger, &param]() { return judger.submit(param); }) };
                    tokens.push_back(std::move(token));
                });
                for (int i{0}; i < tokens.size(); ++i) {
                    auto result{ tokens.at(i).get() };
                    std::lock_guard lock{this->m_TokenWriteMutex};
                    this->m_Token2Id_Index.insert(std::make_pair(result, std::make_pair(schema.submission_id, i)));
                }
            });
        } catch (const std::exception &err)  {
            LOG_ERROR(err.what());
            return false;
        }
        return true;
    }

    std::uint64_t JudgeManager::getSubmissionId(const std::string &token)
    {
        std::lock_guard<std::mutex> lock{ this->m_TokenWriteMutex };
        return this->m_Token2Id_Index.at(token).first;
    }

    void JudgeManager::printToken2Id_Index()
    {
        std::lock_guard<std::mutex> lock{ this->m_TokenWriteMutex };
        for (auto &[key, val]: this->m_Token2Id_Index) {
            LOG_INFO("Token: {}\t    SubId: {}:\t    Index: {}\n", key, std::to_string(val.first), std::to_string(val.second));
        }
    }

    void JudgeManager::insertTaskResult(TaskResult &&task_result)
    {
        std::uint64_t id{};
        {
            std::lock_guard<std::mutex> lock{ this->m_TokenWriteMutex };
            auto [sub_id, idx]{ this->m_Token2Id_Index.at(task_result.token) };
            id = sub_id;
            task_result.index = idx;
            this->m_Token2Id_Index.erase(task_result.token);
        }
        {
            std::lock_guard<std::mutex> lock{ this->m_ResultWriteMutex };
            auto& result{ this->m_SubmissionId2Result.at(id) };
            if(result.insertTaskResult(std::move(task_result))) {
                std::async(std::launch::async, this->m_Callback, std::move(result));
                this->m_SubmissionId2Result.erase(id);
            }
        }
    }
}