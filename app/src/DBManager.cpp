#include "DBManager.hpp"
#include "Logger.hpp"
#include "Core.hpp"

namespace OJApp
{
    DBManager::DBManager(Core::Configurator &cfg)
    {
        int expiry = cfg.get<int>("application", "EXPIRY", 1);
        m_ExpireSec = std::chrono::seconds{expiry};
        m_DBConnPool = std::make_unique<Database::DBPool>(cfg);
    }

    void DBManager::insertOp(std::string table_name, njson json)
    {
        std::shared_ptr<Database::DBWorker> worker{ nullptr };
        do {
            worker = m_DBConnPool->acquire(m_ExpireSec);
        } while (!worker);
        
        std::unique_ptr<sql::PreparedStatement> pstmt{ nullptr };
        if (table_name == "users") {
            pstmt.reset(worker->prepareStatement(R"SQL(INSERT INTO users (user_uuid, username, email, password_hash) VALUES (?, ?, ?, ?))SQL"));
            pstmt->setString(1, json.at("user_uuid").get<std::string>());
            pstmt->setString(2, json.at("username").get<std::string>());
            pstmt->setString(3, json.at("email").get<std::string>());
            pstmt->setString(4, json.at("password_hash").get<std::string>());
        }

        if (pstmt)
            pstmt->executeUpdate();

        m_DBConnPool->returnWorker(worker->workerID());
    }
    std::unique_ptr<sql::ResultSet> DBManager::queryUserInfoByName(std::string_view name)
    {
        std::shared_ptr<Database::DBWorker> worker{ nullptr };
        do {
            worker = m_DBConnPool->acquire(m_ExpireSec);
        } while (!worker);
        
        auto pstmt{ worker->prepareStatement(R"SQL(SELECT * FROM users WHERE username = ?)SQL") };
        pstmt->setString(1, name.data());
        m_DBConnPool->returnWorker(worker->workerID());
        return std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
    }
}