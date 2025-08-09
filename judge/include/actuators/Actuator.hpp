#pragma once
#include "Core.hpp"
#include "Types.hpp"
#include "utilities/ExecutionResult.hpp"
#include "utilities/ResourceLimits.hpp"
#include "Configurator.hpp"

namespace Judge
{
    using Core::Types::TimeStamp;

    class Actuator
    {
    public:
        virtual ~Actuator() = default;

        static void initSystem(Core::Configurator& cfg);
        
        // 核心执行方法
        ExecutionResult execute(const fs::path& exe_path,
                                const ResourceLimits& limits,
                                std::string_view stdin_data);

        
    protected:
        void setChildEnv(int stdin_pipe[2]
                        ,int stdout_pipe[2]
                        ,int stderr_pipe[2]
                        ,const fs::path& exe_path
                        ,const ResourceLimits& limits);
        // 子进程运行逻辑 （ 由具体语言实现 )
        virtual void setupLanguageEnv();
        virtual void runChildProcess(const fs::path& exe_path) = 0;

        // 管道管理
        void createPipes(int stdin_pipe[2], int stdout_pipe[2], int stderr_pipe[2]);
        
        // Cgroup管理
        fs::path createCgroupForProcess(pid_t pid, const ResourceLimits& limit);
        void setupCgroupLimits(const ResourceLimits& limit, const fs::path& cgroup_path);
        void cleanupCgroup(const fs::path& cgroup_path);

        // 子进程监控
        void monitorChild(pid_t child_pid,
                         int stdout_fd,
                         int stderr_fd,
                         const TimeStamp& start_at,
                         ExecutionResult& result,
                         const ResourceLimits& limit,
                         const fs::path& cgroup_path);

        static Core::Configurator* s_ConfiguratorPtr;
    };

    template <typename TActuator, typename... Args>
    requires std::derived_from<TActuator, Actuator>
    inline std::unique_ptr<Actuator> createActuator(Args&&... args) {
        return std::make_unique<TActuator>(std::forward<Args>(args)...);
    }
}