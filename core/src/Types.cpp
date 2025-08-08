#include "Types.hpp"

namespace Core::Types
{
    std::string difficultyToString(DifficultyLevel difficulty)
    {
        switch (difficulty)
        {
        case DifficultyLevel::Beginner: return "Beginner";
        case DifficultyLevel::Basic: return "Basic";
        case DifficultyLevel::Intermediate: return "Intermediate";
        case DifficultyLevel::Advanced: return "Advanced";
        case DifficultyLevel::Expert: return "Expert";
        default: throw std::out_of_range{"unexpected difficulty level"};
        }
    }
}