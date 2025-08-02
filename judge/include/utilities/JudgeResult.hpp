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
        Millis duration{};
        size_t index{};
        int exit_code{};
        int memory_kb{-1};
        int signal{-1};
        TestStatus status{};

        void setResult(ExecutionResult&& er);
    };

    std::string toString(const TestResult& r);


    class JudgeResult
    {
    public:
        JudgeResult(std::string_view problem, SubID sub_id, size_t test_num);
        ~JudgeResult() = default;

        bool insertTestResult(TestResult&& result);

        inline JudgeResult& setCreateTime(TimeStamp create_time) { 
            this->m_CreateAt = create_time;
            return *this;
        }

        inline JudgeResult& setCompileMessage(std::string&& msg) {
            this->m_CompileMsg = std::forward<std::string>(msg);
            return *this;
        }

        inline bool isCompleted() const {
            return m_Results.size() == m_FinishedNum;
        }

        inline SubmissionStatus getStatus() const {
            return this->m_Status;
        }

        std::string toString() const;

    private:
        void setStatus();

    private:
        std::string m_Problem;
        std::string m_CompileMsg{};
        std::vector<TestResult> m_Results;
        TimeStamp m_CreateAt{};
        TimeStamp m_FinishAt{};
        SubID m_SubId{};
        size_t m_FinishedNum{0};
        SubmissionStatus m_Status{ SubmissionStatus::UNKNOWN};
    };
}