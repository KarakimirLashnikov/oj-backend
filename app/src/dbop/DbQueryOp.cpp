#include "dbop/DbQueryOp.hpp"
#include "Logger.hpp"

namespace OJApp::DbOp
{
    DbQueryOp::DbQueryOp(std::string sql, njson param_array)
        : DbOperation{ std::move(sql), std::move(param_array) }
        , m_QueryResult{}
    {
        LOG_INFO("Db QueryOp: {}", m_Sql);
        LOG_INFO(" params: {}", m_ParamArr.dump());
    }
    
    void DbQueryOp::operator()(std::shared_ptr<Database::DbWorker> worker)
    {
        try {
            auto con = worker->getConnPtr();

            auto pstmt = std::unique_ptr<sql::PreparedStatement>(con->prepareStatement(m_Sql));
            if (!pstmt) {
                LOG_ERROR("Op failed: {}, statement is NULL", this->getOpTypeName());
                return;
            }
            this->setSqlVariables(pstmt.get());
            
            m_QueryResult.reset(pstmt->executeQuery());
            LOG_INFO("Op succeed: {}", this->getOpTypeName());
        }
        catch (sql::SQLException &e) {
            LOG_ERROR("Exception: {}, query failed.", e.what());
        }
    }

    njson::array_t DbQueryOp::getResult()
    {
        njson::array_t result_arr = njson::array();
        while (m_QueryResult->next()) {
            njson temp;
            auto col = m_QueryResult->getMetaData()->getColumnCount();
            for (int i{ 1 }; i <= col; ++i) {
                auto name = m_QueryResult->getMetaData()->getColumnName(i);
                temp[name] = m_QueryResult->getString(i);
            }
            result_arr.push_back(temp);
        }
        return result_arr;
    }
}
