#pragma once


namespace Judge
{
    struct ResourceLimits
    {
        float cpu_time_limit_s;
        float cpu_extra_time_s;
        float wall_time_limit_s;
        int memory_limit_kb;
        int stack_limit_kb;
    };
}