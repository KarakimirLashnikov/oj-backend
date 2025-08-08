#include "MySQLDB.hpp"

namespace MySQLDB
{
    Database::Database(std::string_view host, uint16_t port, std::string_view username, std::string_view password, std::string_view database)
        : m_Host{ host }
        , m_Port{ port }
        , m_UserName{ username }
        , m_Password{ password }
        , m_Database{ database }
        , m_ConnPtr{nullptr}
    {
        this->connect();
    }

    Database::~Database()
    {
        if (m_ConnPtr) {
            m_ConnPtr->close();
            m_ConnPtr.reset();
        }
    }

    void Database::connect()
    {
        sql::mysql::MySQL_Driver* driver{ sql::mysql::get_mysql_driver_instance() };
        sql::ConnectOptionsMap connectionOptions;
        connectionOptions["hostName"] = m_Host.data();
        connectionOptions["userName"] = m_UserName.data();
        connectionOptions["password"] = m_Password.data();
        connectionOptions["schema"] = m_Database.data();
        connectionOptions["port"] = m_Port;
        m_ConnPtr.reset(driver->connect(connectionOptions));
    }
}