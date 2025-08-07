#pragma once
#include <mysql_connection.h>
#include <mysql_driver.h>
#include "Core.hpp"

namespace MySQLDB
{
    class Database
    {
    public:
        Database(std::string_view host
                , std::string_view username
                , std::string_view password
                , std::string_view database);

        virtual ~Database();

        virtual void connect();

    protected:
        std::unique_ptr<sql::Connection> m_ConnPtr;
        std::string m_Host;
        std::string m_UserName;
        std::string m_Password;
        std::string m_Database;
    };
}