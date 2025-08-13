#pragma once
#include "dbop/DbOperation.hpp"

namespace OJApp::DbOp
{
    class DbInsertOp : public DbOperation
    {
    public:
        DbInsertOp(std::string sql, njson param_array);
        virtual ~DbInsertOp() noexcept = default;

        OPERATION_TYPE(INSERT)
    };
}