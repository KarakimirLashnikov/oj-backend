#pragma once
#include "MySQLDB.hpp"
#include "Core.hpp"
#include "Types.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Submission.hpp"


namespace JudgeDB
{
    using Core::Types::SubID;
    using Core::Types::TestCase;
    class JudgeWriter : public MySQLDB::Database
    {
    public:
        explicit JudgeWriter(std::string_view host
                            , std::string_view user
                            , std::string_view password
                            , std::string_view database);
        virtual ~JudgeWriter() = default;

        void createSubmission(const Judge::Submission& submission, const std::string& source_code);

        void writeJudgeResult(const Judge::JudgeResult& judge_result);

        void updateSubmission(SubID sub_id, Judge::SubmissionStatus status);

        void insertTestCases(const std::vector<TestCase>& test_cases, std::string_view problem_title);
    };
}