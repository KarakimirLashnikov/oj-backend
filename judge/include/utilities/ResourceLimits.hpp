#pragma once
#include "Core.hpp"

namespace Judge
{
    struct ResourceLimits
    {
        float time_limit_s;
        float extra_time_s;
        float wall_time_s;
        uint32_t memory_limit_kb;
        uint32_t stack_limit_kb;
    };
}