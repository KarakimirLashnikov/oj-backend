#pragma once

#include "Core.hpp"
#include "utilities/Language.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/ResourceLimits.hpp"

namespace Judge
{
    using namespace Core::Types;
    using namespace Language;
    struct Submission
    {
        std::string user_uuid;
        std::string problem_title;
        std::string source_code;
        std::string submission_id;
        LangID language_id;
    };
}