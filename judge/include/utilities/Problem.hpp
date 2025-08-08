#pragma once

#include "Core.hpp"
#include "Types.hpp"
#include "utilities/ResourceLimits.hpp"

namespace Judge
{
    using Core::Types::DifficultyLevel;
    struct Problem
    {
        std::string title;
        std::string description;
        DifficultyLevel level;
        ResourceLimits limits;
    };
}