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
        this->user_uuid = json.at("user_uuid").get<std::string>();
        this->username = json.at("username").get<std::string>();
        this->password_hash = json.at("password_hash").get<std::string>();
        this->email = json.at("email").get<std::string>();
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
            { "author_uuid", this->username },
            { "title", this->title },
            { "description", this->description },
            { "difficulty", difficultyToString(this->difficulty) },
        };
        return j;
    }

    void ProblemInfo::fromJson(const njson& json)
    {
        this->username = json.at("username").get<std::string>();
        this->title = json.at("title").get<std::string>();
        this->description = json.at("description").get<std::string>();
        this->difficulty = stringToDifficulty(json.at("difficulty").get<std::string>());
    }

    njson TestCaseInfo::toJson() const
    {
        njson j = njson{
            { "problem_title", this->problem_title },
            { "stdin_data", this->stdin_data },
            { "expected_output", this->expected_output },
            { "sequence", this->sequence }
        };
        return j;
    }

    void TestCaseInfo::fromJson(const njson& json)
    {
        this->problem_title = json.at("problem_title").get<std::string>();
        this->stdin_data = json.at("stdin_data").get<std::string>();
        this->expected_output = json.at("expected_output").get<std::string>();
        this->sequence = json.at("sequence").get<uint32_t>();
    }


    std::string LanguageIdtoString(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return "C++";
        case LangID::PYTHON:
            return "Python";
        case LangID::UNKNOWN:
        default: return "unknown";
        }
    }

    std::string getFileExtension(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return ".cpp";
        case LangID::PYTHON:
            return ".py";
        case LangID::UNKNOWN:
        default: return "";
        }
    }

    njson LimitsInfo::toJson() const
    {
        njson j = njson{
            { "time_limit_s", this->time_limit_s },
            { "extra_time_s", this->extra_time_s },
            { "wall_time_s", this->wall_time_s },
            { "memory_limit_kb", this->memory_limit_kb },
            { "stack_limit_kb", this->stack_limit_kb }
        };
        return j;
    }

    void LimitsInfo::fromJson(const njson &data)
    {
        this->time_limit_s = data.at("time_limit_s").get<float>();
        this->extra_time_s = data.at("extra_time_s").get<float>();
        this->wall_time_s = data.at("wall_time_s").get<float>();
        this->memory_limit_kb = data.at("memory_limit_kb").get<uint32_t>();
        this->stack_limit_kb = data.at("stack_limit_kb").get<uint32_t>();
    }

    njson SubmissionInfo::toJson() const
    {
        njson j = njson{
            { "username", this->username },
            { "problem_title", this->problem_title },
            { "source_code", this->source_code },
            { "submission_id", this->submission_id },
            { "language", static_cast<int>(this->language_id) }
        };
        return j;
    }

    void SubmissionInfo::fromJson(const njson &data)
    {
        this->username = data.at("username").get<std::string>();
        this->problem_title = data.at("problem_title").get<std::string>();
        this->source_code = data.at("source_code").get<std::string>();
        this->submission_id = data.at("submission_id").get<std::string>();
        this->language_id = static_cast<LangID>(data.at("language").get<int>());
    }

    njson ProblemLimitsInfo::toJson() const
    {
        njson j = LimitsInfo::toJson();
        j["language_id"] = static_cast<int>(this->language_id);
        j["problem_title"] = this->problem_title;
        return j;
    }

    void ProblemLimitsInfo::fromJson(const njson &data)
    {
        this->language_id = static_cast<LangID>(data.at("language_id").get<int>());
        this->problem_title = data.at("problem_title").get<std::string>();
        LimitsInfo::fromJson(data);
    }
}