#include <cppconn/prepared_statement.h>
#include "judgedb/ProblemWriter.hpp"


namespace JudgeDB
{
    ProblemWriter::ProblemWriter(std::string_view host
        , uint16_t port
        , std::string_view user
        , std::string_view password
        , std::string_view database)
        : MySQLDB::Database{ host, port, user, password, database }
    {
    }

    bool ProblemWriter::createProblem(const Judge::Problem& problem)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO problems(title, time_limit_ms, extra_time_ms, wall_time_ms, memory_limit_kb, stack_limit_kb, difficulty, description)
VALUES(?, ?, ?, ?, ?, ?, ?, ?);)SQL"
                )
        );
        pstmt->setString(1, problem.title);
        pstmt->setUInt(2, static_cast<uint32_t>(problem.limits.time_limit_s * 1'000));
        pstmt->setUInt(3, static_cast<uint32_t>(problem.limits.extra_time_s * 1'000));
        pstmt->setUInt(4, static_cast<uint32_t>(problem.limits.wall_time_s * 1'000));
        pstmt->setUInt(5, problem.limits.memory_limit_kb);
        pstmt->setUInt(6, problem.limits.stack_limit_kb);
        pstmt->setString(7, Core::Types::difficultyToString(problem.level));
        pstmt->setString(8, problem.description);
        return pstmt->execute();
    }

    bool ProblemWriter::updateProblem(const Judge::Problem& problem)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
                R"SQL(UPDATE problems
SET time_limit_ms = ?, extra_time_ms = ?, wall_time_ms = ?,ory_limit_kb = ?, stack_limit_kb = ?, difficulty = ?, description = ?
WHERE title = ?;)SQL"
                )
        );
        pstmt->setUInt(1, static_cast<uint32_t>(problem.limits.time_limit_s * 1'000));
        pstmt->setUInt(2, static_cast<uint32_t>(problem.limits.extra_time_s * 1'000));
        pstmt->setUInt(3, static_cast<uint32_t>(problem.limits.wall_time_s * 1'000));
        pstmt->setUInt(4, problem.limits.memory_limit_kb);
        pstmt->setUInt(5, problem.limits.stack_limit_kb);
        pstmt->setString(6, Core::Types::difficultyToString(problem.level));
        pstmt->setString(7, problem.description);
        pstmt->setString(8, problem.title);
        return pstmt->execute();
    }
}