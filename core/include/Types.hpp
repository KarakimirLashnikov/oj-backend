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

    enum class LangID
    {
        UNKNOWN = 0,
        CPP,
        PYTHON
    };

    struct TestCaseInfo : public JsonSerializable
    {
        std::string problem_title;
        std::string stdin_data;
        std::string expected_output;
        uint32_t sequence;

        njson toJson() const override;
        void fromJson(const njson &data) override;
    };

    struct ProblemInfo : public JsonSerializable
    {
        std::string username;
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

    struct LimitsInfo : public JsonSerializable
    {
        float time_limit_s;
        float extra_time_s;
        float wall_time_s;
        uint32_t memory_limit_kb;
        uint32_t stack_limit_kb;

        virtual njson toJson() const override;
        virtual void fromJson(const njson &data) override;
    };

    struct ProblemLimitsInfo : public LimitsInfo
    {
        LangID language_id;
        std::string problem_title;

        njson toJson() const override;
        void fromJson(const njson &data) override;
    };

    struct SubmissionInfo : public JsonSerializable
    {
        std::string username;
        std::string problem_title;
        std::string source_code;
        std::string submission_id;
        LangID language_id;

        njson toJson() const override;
        void fromJson(const njson &data) override;
    };

    std::string difficultyToString(DifficultyLevel diff);
    DifficultyLevel stringToDifficulty(std::string_view str);
    std::string LanguageIdtoString(LangID id);
    std::string getFileExtension(LangID id);
}

namespace bp = boost::process;
namespace fs = std::filesystem;
namespace asio = boost::asio;
