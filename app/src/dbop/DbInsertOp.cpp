#include "dbop/DbInsertOp.hpp"

namespace OJApp::DbOp
{
    DbInsertOp::DbInsertOp(std::string sql, njson param_array)
        : DbOperation{ std::move(sql), std::move(param_array) }
    {}
}
