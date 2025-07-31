#pragma once
#include "Core.hpp"
#include <httplib.h>

namespace Judge
{
    struct SubmissionSchema
    {
        std::uint64_t submission_id{ 0 };
        std::string source_code;
        std::string compiler_options;
        std::vector<std::pair<std::string, std::string>> test_cases;
        std::string callback_url;

        int language_id = -1;
        float cpu_time_limit{ -1.0f };
        float cpu_extra_time{ -1.0f };
        float wall_time_limit{ -1.0f };
        int memory_limit{ -1 };
        int stack_limit{ -1 };

        SubmissionSchema(std::uint64_t id, std::string_view code, int lang_id) noexcept;
        SubmissionSchema(SubmissionSchema &&other) = default;
        SubmissionSchema& operator=(SubmissionSchema&&) = default;
        SubmissionSchema(std::uint64_t id, const SubmissionSchema &other) noexcept;
        ~SubmissionSchema() = default;

        static std::vector<httplib::Params> toHttpParams(const SubmissionSchema& data);

        private:
            SubmissionSchema() = delete;
            SubmissionSchema(SubmissionSchema&) = delete;
            SubmissionSchema& operator=(const SubmissionSchema&) = delete;
    };
}