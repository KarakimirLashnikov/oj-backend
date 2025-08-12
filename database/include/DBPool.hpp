#pragma once
#include "Core.hpp"
#include "DBWorker.hpp"
#include "Configurator.hpp"

namespace Database
{
    class DBPool
    {
    public:
        DBPool(Core::Configurator& cfg);
        ~DBPool() = default;

        std::shared_ptr<DBWorker> acquire(std::chrono::seconds timeout_sec);
        
        // Changed to accept shared_ptr instead of ID
        void returnWorker(size_t id);
    
    private:
        struct WorkerSlot {
            std::shared_ptr<DBWorker> worker;
            bool is_available;
        };

        std::vector<WorkerSlot> m_Pool;
        std::condition_variable m_NotBusyCV;
        std::mutex m_Mtx;
    };
}