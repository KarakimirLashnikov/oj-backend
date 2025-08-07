#pragma once

#include "Core.hpp"
#include "Types.hpp"
namespace Judge
{
    using Core::Types::DifficultyLevel;
    struct Problem
    {
        std::string title;
        float time_limit_s;
        uint32_t memory_limit_kb;
        uint32_t stack_limit_kb;
        DifficultyLevel level;
        std::string description;
    };
}