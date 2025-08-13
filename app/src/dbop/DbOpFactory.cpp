#include "dbop/DbOpFactory.hpp"
#include "ParameterException.hpp"
#include "Logger.hpp"

namespace OJApp::DbOp
{
    using Exceptions::ParameterException;
    std::unique_ptr<DbOperation> makeDbOp(OpType op_type, std::string sql, njson param_array)
    {
        LOG_INFO("sql: {}, params: {}", sql, param_array.dump());
        switch (op_type)
        {
        case OpType::INSERT:
            return std::make_unique<DbInsertOp>(std::move(sql), std::move(param_array));
        case OpType::QUERY:
            return std::make_unique<DbQueryOp>(std::move(sql), std::move(param_array));
        default:
            throw ParameterException(ParameterException::ExceptionType::OUT_OF_RANGE, "Op Type", "invalid op type");
        }
    }
}