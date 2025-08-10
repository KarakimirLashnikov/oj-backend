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

    DifficultyLevel stringToDifficulty(std::string_view str)
    {
        if (str == "Beginner") return DifficultyLevel::Beginner;
        if (str == "Basic") return DifficultyLevel::Basic;
        if (str == "Intermediate") return DifficultyLevel::Intermediate;
        if (str == "Advanced") return DifficultyLevel::Advanced;
        if (str == "Expert") return DifficultyLevel::Expert;
        throw std::out_of_range{ std::string{ "unexpected difficulty level: " } + str.data() };
    }
}