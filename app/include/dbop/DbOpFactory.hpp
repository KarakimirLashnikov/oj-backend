#pragma once
#include "dbop/DbInsertOp.hpp"
#include "dbop/DbQueryOp.hpp"
#include "dbop/DbUpdateOp.hpp"

namespace OJApp::DbOp
{
    std::unique_ptr<DbOperation> makeDbOp(OpType op_type, std::string sql, njson param_array);
}
