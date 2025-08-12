#include "DBPool.hpp"
#include "Core.hpp"
#include "Logger.hpp"

namespace rg = std::ranges;

namespace Database
{
    DBPool::DBPool(Core::Configurator& cfg)
    : m_Pool{}
    , m_NotBusyCV{}
    , m_Mtx{}
    {
        size_t size = cfg.get<size_t>("application", "DB_NUMBER", 2);
        if (size < 1) 
            size = 1; // 确保至少有一个连接
        m_Pool.reserve(size);
        
        for (size_t i = 0; i < size; ++i) {
            auto db = std::make_shared<DBWorker>(
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD", ""),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb"),
                i
            );
            m_Pool.emplace_back(WorkerSlot{db, true});
            LOG_INFO("DBPool initialized worker {}", i);
        }
    }
    
    std::shared_ptr<DBWorker> DBPool::acquire(std::chrono::seconds timeout_sec)
    {
        std::unique_lock<std::mutex> lk{m_Mtx};
        
        // 等待可用连接
        if (!m_NotBusyCV.wait_for(lk, timeout_sec, [this] {
            return std::any_of(m_Pool.cbegin(), m_Pool.cend(), 
                             [](const auto& slot) { return slot.is_available; });
        })) {
            LOG_WARN("DBPool acquire timeout after {} seconds", timeout_sec.count());
            return nullptr;
        }
        
        // 查找第一个可用连接
        auto it = rg::find_if(m_Pool, [](const auto& slot) { 
            return slot.is_available; 
        });
        
        if (it == m_Pool.end()) {
            return nullptr; // 理论上不会走到这里
        }
        
        // 标记为忙碌并检查连接有效性
        it->is_available = false;
        it->worker->checkConnection(); // 确保连接可用
        
        LOG_INFO("DBPool acquired worker {}", it->worker->workerID());
        return it->worker;
    }

    void DBPool::returnWorker(size_t id)
    {
        std::lock_guard<std::mutex> lk{m_Mtx};
        
        if (id >= m_Pool.size()) {
            LOG_ERROR("DBPool return invalid worker id: {}", id);
            return;
        }
        
        m_Pool[id].is_available = true;
        LOG_INFO("DBPool returned worker {}", id);
        m_NotBusyCV.notify_one(); // 通知等待的线程
    }
}
