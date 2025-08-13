#pragma once
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/driver.h>
#include "Core.hpp"

namespace Database
{
    class DbWorker
    {
    public:
        DbWorker(std::string_view host
                , uint16_t port
                , std::string_view user
                , std::string_view password
                , std::string_view database
                , size_t worker_id);
        
        ~DbWorker();

        DbWorker(DbWorker&&) noexcept = default;
        DbWorker& operator=(DbWorker&&) noexcept = default;


        inline sql::Connection* getConnPtr() { return m_Connection.get(); }

        inline size_t workerID() const { return m_ID; }

        // 检查连接有效性，无效则重连
        void checkConnection();
    private:
        // 建立连接，返回是否成功
        bool connect();

        DbWorker(const DbWorker&) = delete;
        DbWorker& operator=(const DbWorker&) = delete;

    private:
        std::unique_ptr<sql::Connection> m_Connection;
        std::string m_Host;
        std::string m_User;
        std::string m_Password;
        std::string m_Database;
        size_t m_ID{};
        uint16_t m_Port;
        // 最大重连次数
        static constexpr int MAX_RECONNECT = 3;
    };
}