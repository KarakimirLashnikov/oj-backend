#pragma once
#include "Core.hpp"
#include "utilities/ExecutionResult.hpp"

namespace Judge
{
    using namespace Core::Types;

    enum class SubmissionStatus : StatusCode
    {
        UNKNOWN = 0,
        PENDING,
        AC, //Accepted
        WA, //Wrong Answer
        RE, //Runtime Error
        MLE, //Memory Limit Exceeded
        TLE, //Time Limit Exceeded
        IE, // Internal Error
        CE, //Compilation Error
    };

    std::string toString(SubmissionStatus st);

    struct TestResult
    {
        std::string stdout{};
        std::string stderr{};
        TimeStamp create_at{};
        TimeStamp exit_at{};
        uint64_t duration_us{0};
        int exit_code{-1};
        int memory_kb{-1};
        int signal{-1};
        TestStatus status;
        
        void setResult(ExecutionResult&& er);

        [[nodiscard]] std::string toString() const;
    };


    struct JudgeResult
    {
    public:
        std::string problem{};
        std::string compile_msg{};
        std::vector<TestResult> results{};
        TimeStamp createAt{};
        TimeStamp finishAt{};
        SubID sub_id{};
        
        void setStatus();
        
        [[nodiscard]] SubmissionStatus status() const { return m_Status; }

        [[nodiscard]] std::string toString() const;
    private:
        SubmissionStatus m_Status{ SubmissionStatus::UNKNOWN};
    };
}