#pragma once
#include "Core.hpp"
#include "utilities/ExecutionResult.hpp"
#include "Types.hpp"

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


    using Core::Types::LangID;
    struct JudgeResult
    {
    public:

        JudgeResult(std::string_view title, std::string_view sub_id, LangID lang_id);

        std::string problem_title{};
        std::string compile_msg{};
        std::vector<TestResult> results{};
        std::string sub_id{};
        LangID language_id;
        
        void setStatus();
        
        [[nodiscard]] SubmissionStatus status() const { return m_Status; }

        [[nodiscard]] std::string toString() const;
    private:
        SubmissionStatus m_Status{ SubmissionStatus::UNKNOWN};
    };
}