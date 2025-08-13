#include "dbop/DbOperation.hpp"
#include "Logger.hpp"
#include "ParameterException.hpp"

namespace OJApp::DbOp
{
    using Exceptions::ParameterException;

    DbOperation::DbOperation(std::string sql, njson param_array)
        : m_Sql{ std::move(sql) }
        , m_ParamArr{ }
    {
        m_ParamArr = std::move(param_array);
        if (!m_ParamArr.is_array()) {
            LOG_ERROR("Paramter is not an array, sql: {}, paramArr: {}", m_Sql, m_ParamArr.dump());
        } else {
            LOG_INFO("sql: {}, params: {}", m_Sql, m_ParamArr.dump());
        }
    }

    void DbOperation::operator()(std::shared_ptr<Database::DbWorker> worker)
    {
        try {
            auto con = worker->getConnPtr();
            con->setAutoCommit(false);

            LOG_INFO(" params: {}", m_Sql, m_ParamArr.dump());
            auto pstmt = std::unique_ptr<sql::PreparedStatement>(con->prepareStatement(m_Sql));

            if (!pstmt) {
                con->setAutoCommit(true);
                LOG_ERROR("Op failed: {}, statement is NULL", this->getOpTypeName());
                return;
            }
            this->setSqlVariables(pstmt.get());

            pstmt->execute();

            con->commit();
            con->setAutoCommit(true);
            LOG_INFO("Op succeed: {}", this->getOpTypeName());
        }
        catch (sql::SQLException &e) {
            auto con = worker->getConnPtr();
            if (con) {
                con->rollback();
                con->setAutoCommit(true); // 恢复自动提交
                LOG_ERROR("Exception: {}, query failed.", e.what());
            }
        }
    }

    void DbOperation::setSqlVariables(sql::PreparedStatement *pstmt)
    {
        size_t i{1};
        for (const auto& param : m_ParamArr)
        {
            LOG_INFO("set Sql Variables {}", param.dump());
            if (param.is_string()) {
                pstmt->setString(i, param.get<std::string>());
            }
            else if (param.is_number_integer()) {
                pstmt->setInt(i, param.get<int>());
            }
            else if (param.is_number_unsigned()) {
                pstmt->setUInt(i, param.get<uint32_t>());
            }
            else if (param.is_number_float()) {
                pstmt->setDouble(i, param.get<double>());
            }
            else if (param.is_boolean()) {
                pstmt->setBoolean(i, param.get<bool>());
            }
            else if (param.is_null()) {
                pstmt->setNull(i, 0);
            }
            else {
                throw Exceptions::ParameterException(
                    ParameterException::ExceptionType::VALUE_ERROR,
                    "Parameters", "Unknown parameter type or value type");
            }
            ++i;
        }
    }
}


