#include <cppconn/prepared_statement.h>
#include "judgedb/UserWriter.hpp"


namespace JudgeDB
{
    UserWriter::UserWriter(std::string_view host
            , std::string_view username
            , std::string_view password
            , std::string_view database)
        : MySQLDB::Database{ host, username, password, database }
    {
    }

    using Core::Types::timeStampToMySQLString;
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