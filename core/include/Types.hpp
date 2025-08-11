#pragma once
#include "Core.hpp"

namespace Core::Types
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    using SubID = boost::uuids::random_generator_pure::result_type;
    using StatusCode = std::uint8_t;

    enum class DifficultyLevel
    {
        Beginner = 1,
        Basic,
        Intermediate,
        Advanced,
        Expert
    };

    struct TestCase
    {
        std::string stdin_data;
        std::string expected_output;
        uint32_t sequence;
    };

    struct ProblemInfo
    {
        std::string author_uuid;
        std::string title;
        std::string description;
        DifficultyLevel level;
    };

    struct UserInfo
    {
        std::string user_uuid;
        std::string username;
        std::string password_hash;
        std::string email;
        std::string salt;
    };

    template <typename T>
    concept IniSupportedType = std::is_integral_v<T> || 
                            std::is_same_v<T, std::string> ||
                            std::is_floating_point_v<T> ||
                            std::is_same_v<T, bool>;

    std::string difficultyToString(DifficultyLevel diff);
    DifficultyLevel stringToDifficulty(std::string_view str);
}

namespace bp = boost::process;
namespace fs = std::filesystem;
namespace asio = boost::asio;
using njson = nlohmann::json;