#pragma once
#include "actuators/Actuator.hpp"

namespace Judge
{
    class CppActuator : public Actuator
    {
    public:
        CppActuator();
        virtual ~CppActuator() noexcept = default;

        ExecutionResult execute(fs::path exe_path,
                                const ResourceLimits &limits,
                                std::string_view stdin_data = "") override;


        static void initSystem();

    private:
        void joinCgroup(const fs::path& cgroup_path);
        void setupCgroupLimits(const ResourceLimits &limits, const fs::path& cgroup_path);
        void setupChildEnv(const ResourceLimits &limits);

        fs::path createCgroupForProcess(pid_t pid, const ResourceLimits& limits);

        void runChildProcess(int stdin_pipe[2], int stdout_pipe[2], int stderr_pipe[2], 
                                      const fs::path& exe_path, const ResourceLimits& limits);

        void monitorChild(pid_t pid,
                          int stdout_fd,
                          int stderr_fd,
                          const TimeStamp &start_time,
                          ExecutionResult &result,
                          const ResourceLimits &limits,
                          const fs::path& cgroup_path);

        void cleanupCgroup(const fs::path& path);
    };
}