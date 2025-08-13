#include "Types.hpp"

namespace Core::Types
{
    njson UserInfo::toJson() const
    {
        njson j = njson{
            { "user_uuid", this->user_uuid },
            { "username", this->username },
            { "password_hash", this->password_hash },
            { "email", this->email }
        };
        return j;
    }

    void UserInfo::fromJson(const njson& json)
    {
        this->user_uuid = json.contains("user_uuid") ? json.at("user_uuid").get<std::string>() : "";
        this->username = json.at("username").get<std::string>();
        this->password_hash = json.at("password_hash").get<std::string>();
        this->email = json.contains("email") ? json.at("email").get<std::string>() : "";
    }


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

    njson ProblemInfo::toJson() const
    {
        njson j = njson{
            { "author_uuid", this->author_uuid },
            { "title", this->title },
            { "description", this->description },
            { "difficulty", difficultyToString(this->difficulty) },
        };
        return j;
    }

    void ProblemInfo::fromJson(const njson& json)
    {
        this->author_uuid = json.at("author_uuid").get<std::string>();
        this->title = json.at("title").get<std::string>();
        this->description = json.at("description").get<std::string>();
        this->difficulty = stringToDifficulty(json.at("difficulty").get<std::string>());
    }

    njson TestCase::toJson() const
    {
        njson j = njson{
            { "stdin_data", this->stdin_data },
            { "expected_output", this->expected_output },
            { "sequence", this->sequence }
        };
        return j;
    }

    void TestCase::fromJson(const njson& json)
    {
        this->stdin_data = json.at("stdin_data").get<std::string>();
        this->expected_output = json.at("expected_output").get<std::string>();
        this->sequence = json.at("sequence").get<uint32_t>();
    }
}