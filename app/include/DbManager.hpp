#pragma once
#include "Types.hpp"
#include "DbPool.hpp"
#include "dbop/DbOperation.hpp"
#include "dbop/DbQueryOp.hpp"

namespace OJApp
{
    using Core::Types::UserInfo;
    class DbManager
    {
    public:
        DbManager(Core::Configurator& cfg);
        ~DbManager() = default;

        void execute(DbOp::DbOperation* db_op);

        njson::array_t query(DbOp::DbQueryOp* query_op);

    private:
        std::unique_ptr<Database::DbPool> m_DBConnPool;
        std::chrono::seconds m_ExpireSec;
    };
}