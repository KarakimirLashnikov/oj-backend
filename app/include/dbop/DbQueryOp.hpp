#pragma once
#include "dbop/DbOperation.hpp"

namespace OJApp::DbOp
{
    class DbQueryOp : public DbOperation
    {
    public:
        DbQueryOp(std::string sql, njson param_array);

        virtual ~DbQueryOp() = default;

        OPERATION_TYPE(Query)

        void operator()(std::shared_ptr<Database::DbWorker>) override;

        njson::array_t getResult();
    private:
        std::unique_ptr<sql::ResultSet> m_QueryResult;
    };
}