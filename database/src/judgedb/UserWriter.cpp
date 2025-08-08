#include <cppconn/prepared_statement.h>
#include "judgedb/UserWriter.hpp"


namespace JudgeDB
{
    UserWriter::UserWriter(std::string_view host
            , uint16_t port
            , std::string_view user
            , std::string_view password
            , std::string_view database)
        : MySQLDB::Database{ host, port, user, password, database }
    {
    }

    bool UserWriter::createUser(std::string_view username
                   , std::string_view email 
                   , std::string_view password)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO users(username, password_hash, email)
VALUES(?, ?, ?);)SQL"
            )
        );
        pstmt->setString(1, username.data());
        pstmt->setString(2, password.data());
        pstmt->setString(3, email.data());
        return pstmt->execute();
    }
}