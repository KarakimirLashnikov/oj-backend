#include "SubmissionSchema.hpp"
#include "Configurator.hpp"

namespace Judge
{
    SubmissionSchema::SubmissionSchema(std::uint64_t id, std::string_view code, int lang_id) noexcept
    : submission_id{ id }, source_code{ code.begin(), code.end() }, language_id{ lang_id }
    {
        Core::Configurator config{ "config.ini" };
        this->cpu_time_limit = config.get<float>("submission-defaults-values", "cpu_time_limit", -1.0f);
        this->cpu_extra_time = config.get<float>("submission-defaults-values", "cpu_extra_time", -1.0f);
        this->wall_time_limit = config.get<int>("submission-defaults-values", "wall_time_limit", -1.0f);
        this->memory_limit = config.get<int>("submission-defaults-values", "memory_limit", -1);
        this->stack_limit = config.get<int>("submission-defaults-values", "stack_limit", -1);
    }

    SubmissionSchema::SubmissionSchema(std::uint64_t id, const SubmissionSchema &other) noexcept
    : submission_id{ id }
    , source_code{ other.source_code }
    , language_id{ other.language_id }
    , compiler_options{ other.compiler_options }
    , test_cases{ other.test_cases }
    , cpu_time_limit{ other.cpu_time_limit }
    , cpu_extra_time{ other.cpu_extra_time }
    , wall_time_limit{ other.wall_time_limit }
    , memory_limit{ other.memory_limit }
    , stack_limit{ other.stack_limit }
    {
    }

    std::vector<httplib::Params> SubmissionSchema::toHttpParams(const SubmissionSchema &data)
    {
        httplib::Params ret{};
        std::vector<httplib::Params> rets{};
        ret.emplace("language_id", std::to_string(data.language_id));
        ret.emplace("source_code", data.source_code);
        if (!data.compiler_options.empty())
            ret.emplace("compiler_options", data.compiler_options);
        if (data.cpu_time_limit > 0)
            ret.emplace("cpu_time_limit", std::to_string(data.cpu_time_limit));
        if (data.cpu_extra_time > 0)
            ret.emplace("cpu_extra_time", std::to_string(data.cpu_extra_time));
        if (data.wall_time_limit > 0)
            ret.emplace("wall_time_limit", std::to_string(data.wall_time_limit));
        if (data.memory_limit > 0)
            ret.emplace("memory_limit", std::to_string(data.memory_limit));
        if (data.stack_limit > 0)
            ret.emplace("stack_limit", std::to_string(data.stack_limit));
        if (!data.test_cases.empty()) {
            for (std::uint64_t i = 0; i < data.test_cases.size(); i++) {
                auto tmp{ ret };
                tmp.emplace("input", data.test_cases[i].first);
                tmp.emplace("expected_output", data.test_cases[i].second);
                rets.push_back(tmp);
            }
        }
        else {
            rets.push_back(ret);
        }
        return rets;
    }
}