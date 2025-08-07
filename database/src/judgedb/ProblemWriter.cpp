#include <cppconn/prepared_statement.h>
#include "judgedb/ProblemWriter.hpp"


namespace JudgeDB
{
    ProblemWriter::ProblemWriter(std::string_view host
        , std::string_view username
        , std::string_view password
        , std::string_view database)
        : MySQLDB::Database{ host, username, password, database }
    {
    }

    bool ProblemWriter::createProblem(const Judge::Problem& problem)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
                R"SQL(INSERT INTO problems(title, time_limit_ms, memory_limit_kb, stack_limit_kb, difficulty, description)
VALUES(?, ?, ?, ?, ?, ?);)SQL"
                )
        );
        pstmt->setString(1, problem.title);
        pstmt->setInt(2, static_cast<int>(problem.time_limit_s * 1'000));
        pstmt->setInt(3, problem.memory_limit_kb);
        pstmt->setInt(4, problem.stack_limit_kb);
        pstmt->setString(5, Core::Types::difficultyToString(problem.level));
        pstmt->setString(6, problem.description);
        return pstmt->execute();
    }

    bool ProblemWriter::updateProblem(const Judge::Problem& problem)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
                R"SQL(UPDATE problems
SET time_limit_ms = ?, memory_limit_kb = ?, stack_limit_kb = ?, difficulty = ?, description = ?
WHERE title = ?;)SQL"
                )
        );
        pstmt->setUInt(1, static_cast<uint32_t>(problem.time_limit_s * 1'000));
        pstmt->setUInt(2, problem.memory_limit_kb);
        pstmt->setUInt(3, problem.stack_limit_kb);
        pstmt->setString(4, Core::Types::difficultyToString(problem.level));
        pstmt->setString(5, problem.description);
        pstmt->setString(6, problem.title);
        return pstmt->execute();
    }
}