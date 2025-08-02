#pragma once
#include "actuators/Actuator.hpp"

namespace Judge
{
    class CppActuator : public Actuator
    {
    public:
        CppActuator() = default;
        virtual ~CppActuator() noexcept = default;

        ExecutionResult execute(fs::path exe_path,
                                const ResourceLimits &limits,
                                std::string_view stdin_data = "") override;

    private:
        void setupChildEnv(const ResourceLimits &limits);

        void allowSyscalls(scmp_filter_ctx ctx);

        void lanchProcess(bp::pipe &stdin_pipe,
                          bp::pipe &stdout_pipe,
                          bp::pipe &stderr_pipe,
                          const fs::path &exe_path);

        void setupCgroups(pid_t pid, const ResourceLimits &limits, const fs::path& cgroup_path);

        void monitorChild(pid_t pid,
                          bp::ipstream &stdout_stream,
                          bp::ipstream &stderr_stream,
                          const TimeStamp &start_time,
                          ExecutionResult &result,
                          const ResourceLimits &limits,
                          const fs::path& cgroup_path);
    };
}