#pragma once
#include "Core.hpp"
#include "JsonSerializable.hpp"

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

    struct TestCase : public JsonSerializable
    {
        std::string stdin_data;
        std::string expected_output;
        uint32_t sequence;

        njson toJson() const override;
        void fromJson(const njson &data) override;
    };

    struct ProblemInfo : public JsonSerializable
    {
        std::string author_uuid;
        std::string title;
        std::string description;
        DifficultyLevel difficulty;

        njson toJson() const override;
        void fromJson(const njson &data) override;
    };

    struct UserInfo : public JsonSerializable
    {
        std::string user_uuid;
        std::string username;
        std::string password_hash;
        std::string email;

        njson toJson() const override;
        void fromJson(const njson &data) override;
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
