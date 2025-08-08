#pragma once
#include "MySQLDB.hpp"
#include "utilities/Problem.hpp"

namespace JudgeDB
{
    class ProblemInquirer : public MySQLDB::Database
    {
    public:
        ProblemInquirer(std::string_view host
                        , uint16_t port
                        , std::string_view user
                        , std::string_view password
                        , std::string_view database);

        virtual ~ProblemInquirer() = default;

        bool isExist(std::string_view title);

        std::vector<Judge::Problem> getAllProblems();

        Judge::Problem getProblem(std::string_view title);

        uint32_t getProblemID(std::string_view title);
    };
}