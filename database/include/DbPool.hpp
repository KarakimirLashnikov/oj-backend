#pragma once
#include "Core.hpp"
#include "DbWorker.hpp"
#include "Configurator.hpp"

namespace Database
{
    class DbPool
    {
    public:
        DbPool(Core::Configurator& cfg);
        ~DbPool() = default;

        std::shared_ptr<DbWorker> acquire(std::chrono::seconds timeout_sec);
        
        // Changed to accept shared_ptr instead of ID
        void returnWorker(size_t id);
    
    private:
        struct WorkerSlot {
            std::shared_ptr<DbWorker> worker;
            bool is_available;
        };

        std::vector<WorkerSlot> m_Pool;
        std::condition_variable m_NotBusyCV;
        std::mutex m_Mtx;
    };
}