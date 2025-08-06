#pragma once

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include "Core.hpp"
#include "Types.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Submission.hpp"


namespace JudgeDB
{
    using Core::Types::SubID;
    class JudgeWriter
    {
    public:
        explicit JudgeWriter(std::string_view host
                            , std::string_view user
                            , std::string_view password
                            , std::string_view database);
        ~JudgeWriter();

        void createSubmission(const Judge::Submission& submission, const std::string& source_code);

        void writeJudgeResult(const Judge::JudgeResult& judge_result);

        void updateSubmission(SubID sub_id, Judge::SubmissionStatus status);

    private:
        std::unique_ptr<sql::Connection> m_Conn;
        std::string m_Host;
        std::string m_User;
        std::string m_Password;
        std::string m_Database;

        void connect();
    };

}