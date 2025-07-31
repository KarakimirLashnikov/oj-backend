#include "JudgeManager.hpp"
#include "Judger.hpp"

namespace Judge
{
    JudgeManager::JudgeManager(std::string_view url,
                               std::string_view auth_token,
                               std::function<void(SubmissionResult &&)> callback,
                               int max_workers)
        : m_JudgeUrl{ url.data() }
        , m_AuthToken{ auth_token.data() }
        , m_Callback{ callback }
        , m_Workers{ std::make_unique<Core::ThreadPool>(static_cast<std::size_t>(max_workers)) }
    {
    }

    bool JudgeManager::submit(SubmissionSchema &&schema)
    {
        this->m_SubmissionId2Result.insert(
            { std::make_pair(schema.submission_id, SubmissionResult(schema.submission_id, schema.test_cases.size()))}
        );

        this->m_Workers->enqueue([this, schema = std::move(schema)]() -> void {
            auto params{ SubmissionSchema::toHttpParams(schema) };
            Judger judger{ this->m_JudgeUrl, this->m_AuthToken, 10, 5000.0 };
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

    }
}