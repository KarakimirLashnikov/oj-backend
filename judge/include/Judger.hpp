#pragma once
#include "Types.hpp"
#include "actuators/Actuator.hpp"
#include "compilers/Compiler.hpp"

namespace Judge
{
    using Core::Types::LangID;
    using Core::Types::LimitsInfo;
    
    class Judger
    {
    public:
        explicit Judger(LangID language_id, const fs::path& source_code, const LimitsInfo& resource_limits);

        TestResult judge(const std::string& stdin_data, const std::string& expected_output);

        std::optional<std::string> getCompileError() const;
    private:
        std::unique_ptr<Compiler> m_CompilerPtr;
        std::unique_ptr<Actuator> m_ActuatorPtr;
        fs::path m_ExecutablePath{};
        std::string m_CompileError{};
        LimitsInfo m_LimitsInfo{};
    };
}