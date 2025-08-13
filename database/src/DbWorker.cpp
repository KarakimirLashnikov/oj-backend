#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include "DbWorker.hpp"
#include "Logger.hpp"
#include <chrono>
#include <thread>

namespace Database
{
    DbWorker::DbWorker(std::string_view host, uint16_t port, std::string_view user, std::string_view password, std::string_view database, size_t worker_id)
        : m_Host{ host }
        , m_Port{ port }
        , m_User{ user }
        , m_Password{ password }
        , m_Database{ database }
        , m_Connection{ nullptr }
        , m_ID{ worker_id }
    {
        checkConnection(); // 初始化时检查连接
    }

    DbWorker::~DbWorker()
    {
        if (m_Connection) 
        {
            try {
                m_Connection->close();
                LOG_INFO("DBWorker {} connection closed", m_ID);
            } catch (const sql::SQLException& e) {
                LOG_ERROR("DBWorker {} close error: {}", m_ID, e.what());
            }
            m_Connection.reset();
        }
    }

    bool DbWorker::connect()
    {
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            sql::ConnectOptionsMap options;
            options["hostName"] = m_Host;
            options["userName"] = m_User;
            options["password"] = m_Password;
            options["schema"] = m_Database;
            options["port"] = m_Port;
            // 设置连接超时
            options["connectTimeout"] = 5;

            m_Connection.reset(driver->connect(options));
            LOG_INFO("DBWorker {} connected to {}:{}", m_ID, m_Host, m_Port);
            return true;
        } catch (const sql::SQLException& e) {
            LOG_ERROR("DBWorker {} connect failed: {} (error code: {})", m_ID, e.what(), e.getErrorCode());
            m_Connection.reset();
            return false;
        }
    }

    void DbWorker::checkConnection()
    {
        bool need_reconnect = false;

        if (!m_Connection) {
            need_reconnect = true;
        } else {
            try {
                // 检查连接是否有效
                if (!m_Connection->isValid()) {
                    LOG_WARN("DBWorker {} connection is invalid", m_ID);
                    need_reconnect = true;
                }
            } catch (const sql::SQLException& e) {
                LOG_ERROR("DBWorker {} connection check failed: {}", m_ID, e.what());
                need_reconnect = true;
            }
        }

        if (need_reconnect) {
            int retry = 0;
            while (retry < MAX_RECONNECT && !connect()) {
                retry++;
                LOG_INFO("DBWorker {} reconnecting ({} of {})...", m_ID, retry, MAX_RECONNECT);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}
