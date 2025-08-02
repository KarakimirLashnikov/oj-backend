#pragma once
#include <seccomp.h>
#include "Core.hpp"
#include "Types.hpp"
#include "utilities/ExecutionResult.hpp"
#include "utilities/ResourceLimits.hpp"

namespace Judge
{
    using Core::Types::TimeStamp;

    class Actuator
    {
    public:
        virtual ~Actuator() = default;
        virtual ExecutionResult execute(fs::path exe_path,
                                        const ResourceLimits& limits,
                                        std::string_view stdin_data = "") = 0;
    };

    template <typename TActuator, typename... Args>
    requires std::derived_from<TActuator, Actuator>
    inline std::unique_ptr<Actuator> createActuator(Args&&... args) {
        return std::make_unique<TActuator>(std::forward<Args>(args)...);
    }
}