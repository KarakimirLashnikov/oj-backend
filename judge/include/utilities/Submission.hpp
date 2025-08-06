#pragma once

#include "Core.hpp"
#include "utilities/Language.hpp"
#include "utilities/JudgeResult.hpp"

namespace Judge
{
    using namespace Core::Types;
    using namespace Language;
    struct Submission
    {
        std::string user_name;
        std::string problem_title;
        std::variant<std::monostate,
                     std::string, 
                     fs::path>
            uploade_code_file_path;
        std::vector<std::string> compile_options;
        SubID submission_id;
        float cpu_time_limit_s;
        float cpu_extra_time_s;
        float wall_time_limit_s;
        int memory_limit_kb;
        int stack_limit_kb;
        LangID language_id;
    };
}