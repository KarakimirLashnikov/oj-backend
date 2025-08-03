#include "utilities/JudgeResult.hpp"
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
        case SubmissionStatus::AC: return "AC";
        case SubmissionStatus::WA: return "WA";
        case SubmissionStatus::RE: return "RE";
        case SubmissionStatus::MLE: return "MLE";
        case SubmissionStatus::TLE: return "TLE";
        case SubmissionStatus::IE: return "IE";
        case SubmissionStatus::CE: return "CE";
        default: throw std::runtime_error("Undefined submission status.");
        }
    }

    std::string toString(const TestResult &r)
    {
        std::string result{ std::format("[\n  Index: {}\n  Status: {}\n  Runtime: {} ms\n  Memory: {} KB\n  Exit Code: {}\n  Exit Signal: {}\n", r.index, toString(r.status), r.duration.count(), r.memory_kb, r.exit_code, r.signal) };
        result.append("    Output: " + (r.stdout.size() > 10 ? r.stdout.substr(10) + "... \n" : r.stdout + "\n"));
        result.append("    Stderr: " + (r.stderr.size() > 10 ? r.stderr.substr(10) + "... \n" : r.stderr + "\n"));
        result.append("]\n");
        return result;
    }

    void Judge::TestResult::setResult(ExecutionResult &&er)
    {
        this->create_at = er.create_at;
        this->duration = std::chrono::duration_cast<Millis>(er.finish_at - er.create_at);
        this->exit_at = er.finish_at;
        this->memory_kb = er.memory_kb;
        this->signal = er.signal;
        this->stdout = std::move(er.stdout);
        this->stderr = std::move(er.stderr);
        this->exit_code = er.exit_code;
        this->status = er.status;
    }

    JudgeResult::JudgeResult(std::string_view problem, SubID sub_id, size_t test_num)
    : m_Problem{ problem }
    , m_SubId{ sub_id }
    , m_Results{ test_num }
    {
    }

    bool JudgeResult::insertTestResult(TestResult &&test_result)
    {
        ++this->m_FinishedNum;
        if (this->isCompleted()) {
            this->m_FinishAt = std::chrono::steady_clock::now();
            this->setStatus();
        }
        auto& test_slot = this->m_Results.at(test_result.index);
        if (test_slot.status != TestStatus::UNKNOWN)
            return false;
        test_slot = std::move(test_result);
        return true;
    }

    void JudgeResult::setStatus()
    {
        StatusCode status_map = std::ranges::fold_left(
            this->m_Results, 0, [](StatusCode status, const auto &task_result) -> StatusCode {
            return status | static_cast<StatusCode>(task_result.status);
        });

        if (status_map == static_cast<StatusCode>(TestStatus::ACCEPTED)) {
            this->m_Status = SubmissionStatus::AC;
        } else if (status_map == static_cast<StatusCode>(TestStatus::COMPILATION_ERROR)) {
            this->m_Status = SubmissionStatus::CE;
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
        std::string str{ std::format("Problem: {}\n", this->m_Problem)};
        str.append("SubmissionId: " + boost::uuids::to_string(this->m_SubId) + "\n");
        str.append("Status: " + Judge::toString(this->m_Status) + "\n");
        str.append("Duration: " + std::to_string(std::chrono::duration_cast<Millis>(this->m_FinishAt - this->m_CreateAt).count()) + " ms\n");
        str.append("Compile Message: " + this->m_CompileMsg + "\n");
        str.append("TestResults: ");
        for (const auto &result: this->m_Results) {
            str.append(Judge::toString(result) + "\n");
        }
        return str;

    }
}