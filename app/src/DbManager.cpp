#include "DbManager.hpp"
#include "Logger.hpp"
#include "Core.hpp"
#include "SystemException.hpp"
#include "ParameterException.hpp"

namespace OJApp
{
    using Exceptions::SystemException;
    using Exceptions::makeSystemException;
    using Exceptions::ParameterException;

    DbManager::DbManager(Core::Configurator &cfg)
    {
        int expiry = cfg.get<int>("judgedb", "OP_WAIT_TIME", 10);
        m_ExpireSec = std::chrono::seconds{expiry};
        m_DBConnPool = std::make_unique<Database::DbPool>(cfg);
    }

    void DbManager::execute(DbOp::DbOperation* db_op)
    {
        std::shared_ptr<Database::DbWorker> worker{ nullptr };
        do {
            worker = m_DBConnPool->acquire(m_ExpireSec);
        } while (!worker);

        try {
            if (db_op)
                (*db_op)(worker);
            else
                LOG_ERROR("Invalid Db operation, db op is NULL");
        } catch (const Exceptions::ParameterException& e) {
            m_DBConnPool->returnWorker(worker->workerID());
            LOG_ERROR("Db Manager Exception: {}", e.what());
        } catch (const sql::SQLException &e) {
            if (e.getErrorCode() == 1205 || e.getErrorCode() == 1213)
            {
                LOG_ERROR("db exception: 1205, 1213, release db lock.");
                LOG_WARN("retry DB operate.");
                m_DBConnPool->returnWorker(worker->workerID());
                execute(db_op);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Db Manager Exception: {}", e.what());
        }
        
        m_DBConnPool->returnWorker(worker->workerID());
    }

    njson::array_t DbManager::query(DbOp::DbQueryOp *query_op)
    {
        this->execute(query_op);
        return query_op->getResult();
    }
}