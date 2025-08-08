#pragma once
#include "MySQLDB.hpp"
#include "Types.hpp"
#include "utilities/Problem.hpp"

namespace JudgeDB
{
    using Core::Types::DifficultyLevel;
    class ProblemWriter : public MySQLDB::Database
    {
    public:
        ProblemWriter(std::string_view host
                    , uint16_t port
                    , std::string_view user
                    , std::string_view password
                    , std::string_view database);

        virtual ~ProblemWriter() = default;

        bool createProblem(const Judge::Problem& problem);

        bool updateProblem(const Judge::Problem& problem);
    };
}