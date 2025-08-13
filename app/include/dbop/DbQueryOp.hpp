#pragma once
#include "dbop/DbOperation.hpp"

namespace OJApp::DbOp
{
    class DbQueryOp : public DbOperation
    {
    public:
        DbQueryOp(std::string sql, njson param_array);

        virtual ~DbQueryOp() = default;

        OPERATION_TYPE(QUERY)

        void operator()(std::shared_ptr<Database::DbWorker>) override;

        njson getResult();
    private:
        std::unique_ptr<sql::ResultSet> m_QueryResult;
    };
}