#pragma once
#include "Core.hpp"
#include "actuators/Actuator.hpp"
#include "compilers/Compiler.hpp"
#include "utilities/Language.hpp"

namespace Judge
{
    using Language::LangID;
    class Judger
    {
    public:
        explicit Judger(LangID language_id, const fs::path& source_code, const ResourceLimits& resource_limits);

        TestResult judge(const std::string& stdin_data, const std::string& expected_output);

        std::optional<std::string> getCompileError() const;
    private:
        std::unique_ptr<Compiler> m_CompilerPtr;
        std::unique_ptr<Actuator> m_ActuatorPtr;
        fs::path m_ExecutablePath{};
        std::string m_CompileError{};
        ResourceLimits m_ResourceLimits{};
    };
}