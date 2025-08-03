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
        std::string problem;
        std::string source_code_path;
        std::vector<std::string> compile_options;
        TestCases test_cases;
        SubID submission_id;
        float cpu_time_limit_s;
        float cpu_extra_time_s;
        float wall_time_limit_s;
        int memory_limit_kb;
        int stack_limit_kb;
        LangID language_id;
    };
}