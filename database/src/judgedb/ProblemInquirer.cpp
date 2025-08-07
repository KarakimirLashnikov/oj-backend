#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "Core.hpp"
#include "judgedb/ProblemInquirer.hpp"

namespace JudgeDB
{
    ProblemInquirer::ProblemInquirer(std::string_view host
        , std::string_view username
        , std::string_view password
        , std::string_view database)
        : MySQLDB::Database{ host, username, password, database }
    {
    }

    bool ProblemInquirer::isExist(std::string_view title)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(SELECT * FROM problems WHERE title = ?;)SQL"
            )
        );
        pstmt->setString(1, title.data());
        auto result = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
        return result->next();
    }

    std::vector<Judge::Problem> ProblemInquirer::getAllProblems()
    {
        std::vector<Judge::Problem> problems;
        std::unique_ptr<sql::Statement> stmt{ m_ConnPtr->createStatement() };
        auto result = std::unique_ptr<sql::ResultSet>(
            stmt->executeQuery(
R"SQL(SELECT * FROM problems ORDER BY id ASC;)SQL"
            )
        );
        while (result->next()) {
            Judge::Problem problem{
                .title = result->getString("title"),
                .time_limit_s = static_cast<float>(result->getUInt("time_limit_ms")) / 1'000,
                .memory_limit_kb = result->getUInt("memory_limit_kb"),
                .stack_limit_kb = result->getUInt("stack_limit_kb"),
                .description = result->getString("description")
            };
            problems.push_back(std::move(problem));
        }
        return problems;
    }

    Judge::Problem ProblemInquirer::getProblem(std::string_view title)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(SELECT * FROM problems WHERE title = ?;)SQL"
            )
        );
        pstmt->setString(1, title.data());
        auto result = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
        if (!result->next()) {
            throw sql::SQLException{ "No problem with this title." };
        }
        Judge::Problem problem{
            .title = result->getString("title"),
            .time_limit_s = static_cast<float>(result->getUInt("time_limit_ms")) / 1'000,
            .memory_limit_kb = result->getUInt("memory_limit_kb"),
            .stack_limit_kb = result->getUInt("stack_limit_kb"),
            .description = result->getString("description")
        };
        return problem;
    }

    uint32_t ProblemInquirer::getProblemID(std::string_view title)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(SELECT id FROM problems WHERE title = ?;)SQL"
            )
        );
        pstmt->setString(1, title.data());
        auto result = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
        if (!result->next()) {
            throw sql::SQLException{ "No problem with this title." };
        }
        return result->getInt("id");
    }
}
