#pragma once
#include "Types.hpp"
#include "DBPool.hpp"


namespace OJApp
{
    class DBManager
    {
    public:
        DBManager(Core::Configurator& cfg);
        ~DBManager() = default;

        void insertOp(std::string table_name, njson json);

        std::unique_ptr<sql::ResultSet> queryUserInfoByName(std::string_view name);

    private:
        std::unique_ptr<Database::DBPool> m_DBConnPool;
        std::chrono::seconds m_ExpireSec;
    };
}