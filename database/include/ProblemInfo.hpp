#pragma once

#include "Core.hpp"
#include "Types.hpp"

namespace Database
{
    using Core::Types::DifficultyLevel;
    struct ProblemInfo
    {
        std::string title;
        std::string description;
        std::string author;
        DifficultyLevel level;
    };
}