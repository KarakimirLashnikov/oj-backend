#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include "judgedb/UserInquirer.hpp"

namespace JudgeDB
{
    UserInquirer::UserInquirer(std::string_view host
        , std::string_view username
        , std::string_view password
        , std::string_view database)
        : MySQLDB::Database{ host, username, password, database }
    {
    }

    bool UserInquirer::isExist(std::string_view username) const
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(SELECT username
FROM users
WHERE username=?;)SQL"
            )
        );

        pstmt->setString(1, username.data());
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        return res->next();
    }

    std::string UserInquirer::getPassword(std::string_view username) const
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(SELECT password_hash
FROM users
WHERE username=?;)SQL"
            )
        );
        pstmt->setString(1, username.data());
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            return std::string{
                res->getString("password_hash")
            };
        }
        throw sql::SQLException("user not exists");
    }
}

