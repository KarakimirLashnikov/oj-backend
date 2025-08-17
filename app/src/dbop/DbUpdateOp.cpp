#include "dbop/DbUpdateOp.hpp"

namespace OJApp::DbOp
{
    DbUpdateOp::DbUpdateOp(std::string sql, njson param_array)
        : DbOperation{std::move(sql), std::move(param_array)}
    {}
}