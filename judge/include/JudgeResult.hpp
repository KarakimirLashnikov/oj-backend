#pragma once

#include "Core.hpp"

namespace Judge
{
#define BITMASK(x) (1U << (x))

    enum class TaskStatus : std::int16_t
    {
        ACCEPTED            = BITMASK(0),
        WRONG_ANSWER        = BITMASK(1),
        TIME_LIMIT_EXCEEDED = BITMASK(2),
        COMPILATION_ERROR   = BITMASK(3),
        SIGSEGV_ERROR       = BITMASK(4),
        SIGXFSZ_ERROR       = BITMASK(5),
        SIGFPE_ERROR        = BITMASK(6),
        SIGABRT_ERROR       = BITMASK(7),
        NZEC_ERROR          = BITMASK(8),
        OTHER_RUNTIME_ERROR = BITMASK(9),
        INTERNAL_ERROR      = BITMASK(10),
        EXEC_FORMAT_ERROR   = BITMASK(11)
    };

    inline constexpr const char* TaskStatusToString(TaskStatus status)
    {
        switch (status)
        {
        case TaskStatus::ACCEPTED: return "Accepted";
        case TaskStatus::WRONG_ANSWER: return "Wrong Answer";
        case TaskStatus::TIME_LIMIT_EXCEEDED: return "Time Limit Exceeded";
        case TaskStatus::COMPILATION_ERROR: return "Compilation Error";
        case TaskStatus::SIGSEGV_ERROR: return "Runtime Error (SIGSEGV)";
        case TaskStatus::SIGXFSZ_ERROR: return "Runtime Error (SIGXFSZ)";
        case TaskStatus::SIGFPE_ERROR: return "Runtime Error (SIGFPE)";
        case TaskStatus::SIGABRT_ERROR: return "Runtime Error (SIGABRT)";
        case TaskStatus::NZEC_ERROR: return "Runtime Error (NZEC)";
        case TaskStatus::OTHER_RUNTIME_ERROR: return "Runtime Error (Other)";
        case TaskStatus::INTERNAL_ERROR: return "Internal Error";
        case TaskStatus::EXEC_FORMAT_ERROR: return "Exec Format Error";
        default: return "Unknown";
        }
    }

    enum class SubmissionStatus : std::int8_t
    {
        AC = 0, //Accepted
        WA, //Wrong Answer
        RE, //Runtime Error
        CE, //Compilation Error
        TLE, //Time Limit Exceeded
        IE, // Internal Error
        PENDING,
        UNKNOWN
    };

    inline constexpr const char* SubmissionStatusToString(SubmissionStatus status)
    {
        switch (status)
        {
        case SubmissionStatus::AC: return "Accepted";
        case SubmissionStatus::WA: return "Wrong Answer";
        case SubmissionStatus::RE: return "Runtime Error";
        case SubmissionStatus::CE: return "Compilation Error";
        case SubmissionStatus::TLE: return "Time Limit Exceeded";
        case SubmissionStatus::IE: return "Internal Error";
        case SubmissionStatus::UNKNOWN: return "Unknown";
        default: return "Unknown";
        }
    }

    struct TaskResult
    {
        int index{-1};
        int exit_code{1};
        int exit_signal{1};
        float time{0.0f};
        float wall_time{0.0f};
        float memory{0.0f};
        std::string stdin;
        std::string expected_output;
        std::string stdout;
        std::string stderr;
        std::string compile_output;
        std::string token;
        std::int16_t status{static_cast<std::int16_t>(TaskStatus::WRONG_ANSWER)};

        std::string toString() const;
    };

    class SubmissionResult
    {
    public:
        using TimeStamp = std::chrono::system_clock::time_point;

        SubmissionResult(std::uint64_t submission_id, int test_count);
        ~SubmissionResult() = default;

        // return true if completed( task_results.size() == test_count )
        bool insertTaskResult(TaskResult&& task_result);
    private:
        void setStatus();

        std::uint64_t m_SubmissionId{0};
        std::vector<TaskResult> m_TaskResults;
        int m_TaskCount{-1};
        int m_FinishedCount{0};
        SubmissionStatus m_Status{SubmissionStatus::UNKNOWN};
        TimeStamp m_CreateAt;
        TimeStamp m_FinishAt;
    };
}