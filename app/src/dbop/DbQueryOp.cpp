#include <cppconn/resultset_metadata.h>
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

    njson::array_t DbQueryOp::getResult() {
        njson::array_t result_arr;
        auto meta = m_QueryResult->getMetaData();
        
        while (m_QueryResult->next()) {
            njson row;
            for (int i = 1; i <= meta->getColumnCount(); ++i) {
                std::string name = meta->getColumnName(i);
                
                if (m_QueryResult->isNull(i)) {
                    row[name] = nullptr;
                    continue;
                }

                switch (meta->getColumnType(i)) {  // 使用 Connector/C++ 提供的类型枚举
                    case sql::DataType::BIT:
                    case sql::DataType::TINYINT:
                        row[name] = m_QueryResult->getBoolean(i);
                        break;
                        
                    case sql::DataType::SMALLINT:
                    case sql::DataType::MEDIUMINT:
                    case sql::DataType::INTEGER:
                        row[name] = m_QueryResult->getInt(i);
                        break;
                        
                    case sql::DataType::BIGINT:
                        row[name] = static_cast<int64_t>(m_QueryResult->getInt64(i));
                        break;
                        
                    case sql::DataType::REAL:
                    case sql::DataType::DOUBLE:
                        row[name] = m_QueryResult->getDouble(i);
                        break;
                        
                    case sql::DataType::DECIMAL:
                    case sql::DataType::NUMERIC:
                        // 高精度数值建议按字符串处理避免精度丢失
                        row[name] = std::stod(m_QueryResult->getString(i));
                        break;
                        
                    case sql::DataType::JSON:
                        // 尝试解析 JSON 字符串
                        try {
                            row[name] = njson::parse(m_QueryResult->getString(i).c_str());
                        } catch (...) {
                            row[name] = m_QueryResult->getString(i);
                        }
                        break;
                        
                    default:  // 包括 CHAR/VARCHAR/TEXT/TIMESTAMP 等
                        row[name] = m_QueryResult->getString(i);
                }
            }
            result_arr.push_back(row);
        }
        return result_arr;
    }
}
