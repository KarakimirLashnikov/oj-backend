#include "utilities/JudgeResult.hpp"
#include "utilities/ExecutionResult.hpp"
#include "Types.hpp"

namespace Judge
{
    using Core::Types::SubID;
    using Core::Types::StatusCode;

    std::string toString(SubmissionStatus s)
    {
        switch (s)
        {
        case SubmissionStatus::UNKNOWN: return "UNKNOWN";
        case SubmissionStatus::PENDING: return "PENDING";
        case SubmissionStatus::AC: return "ACCEPT";
        case SubmissionStatus::WA: return "WRONG_ANSWER";
        case SubmissionStatus::RE: return "RUNTIME_ERROR";
        case SubmissionStatus::MLE: return "MEMORY_LIMIT_EXCEEDED";
        case SubmissionStatus::TLE: return "TIME_LIMIT_EXCEEDED";
        case SubmissionStatus::IE: return "INTERNAL_ERROR";
        case SubmissionStatus::CE: return "COMPILATION_ERROR";
        default: throw std::runtime_error("Undefined submission status.");
        }
    }

    JudgeResult::JudgeResult(std::string_view title, std::string_view sid)
        : problem_title{ title.data() }
        , sub_id{ sid.data() }
    {}

    void JudgeResult::setStatus()
    {
        if (this->results.empty()) {
            this->m_Status = SubmissionStatus::CE;
            return;
        }

        StatusCode status_map = std::ranges::fold_left(
            this->results, 0, [](StatusCode status, const auto &task_result) -> StatusCode {
            return status | static_cast<StatusCode>(task_result.status);
        });

        if (status_map == static_cast<StatusCode>(TestStatus::ACCEPTED)) {
            this->m_Status = SubmissionStatus::AC;
        } else if (status_map & static_cast<StatusCode>(TestStatus::WRONG_ANSWER)) {
            this->m_Status = SubmissionStatus::WA;
        } else if (status_map & static_cast<StatusCode>(TestStatus::MEMORY_LIMIT_EXCEEDED)) {
            this->m_Status = SubmissionStatus::MLE;
        } else if (status_map & static_cast<StatusCode>(TestStatus::TIME_LIMIT_EXCEEDED)) {
            this->m_Status = SubmissionStatus::TLE;
        }  else if (status_map & static_cast<StatusCode>(TestStatus::INTERNAL_ERROR)) {
            this->m_Status = SubmissionStatus::IE;
        } else {
            this->m_Status = SubmissionStatus::RE;
        }
    }

    std::string JudgeResult::toString() const
    {
        std::string str{ std::format("Problem: {}\n", this->problem_title)};
        str.append(std::string("SubmissionId: ") + this->sub_id.data() + "\n");
        str.append("Status: " + Judge::toString(this->m_Status) + "\n");
        str.append("Compile Message: " + this->compile_msg + "\n");
        str.append("TestResults: ");
        for (const auto &result: this->results) {
            str.append(result.toString() + "\n");
        }
        return str;
    }
}