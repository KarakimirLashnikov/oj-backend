#pragma once
#include "Core.hpp"
#include "Types.hpp"
#include "Logger.hpp"
#include "JudgeResult.hpp"
#include "utilities/Submission.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/ResourceLimits.hpp"
#include "utilities/ExecutionResult.hpp"
#include "utilities/Language.hpp"
#include "actuators/Actuator.hpp"
#include "compilers/Compiler.hpp"


namespace Judge
{
    using Core::Types::TestCase;
    using Core::Types::ExpectedOutput;
    using Core::Types::TestInput;
    using Judge::Language::LangID;
    class Judger
    {
    public:
        Judger() = default;
        ~Judger() = default;

        JudgeResult judge(Submission&& submission);

    private:
        std::unique_ptr<Compiler> getCompiler(LangID lang_id);
        std::unique_ptr<Actuator> getActuator(LangID lang_id);
        TestResult judgeTest(const ExpectedOutput& expected_output, TestResult&& result);
    };
}