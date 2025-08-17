#pragma once
#include "dbop/DbOperation.hpp"

namespace OJApp::DbOp
{
    class DbUpdateOp : public DbOperation
    {
    public:
        DbUpdateOp(std::string sql, njson param_array);
        virtual ~DbUpdateOp() = default;

        OPERATION_TYPE(Update)
    };
}