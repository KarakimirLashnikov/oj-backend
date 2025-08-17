#include "Judger.hpp"
#include "actuators/CppActuator.hpp"
#include "actuators/PythonActuator.hpp"
#include "compilers/CppCompiler.hpp"

namespace Judge
{
    Judger::Judger(LangID language_id, const fs::path& source_code, const LimitsInfo& resource_limits)
    : m_ActuatorPtr{}
    , m_CompilerPtr{}
    , m_CompileError{}
    , m_ExecutablePath{ source_code }
    , m_LimitsInfo{resource_limits}
    {
        if (language_id == LangID::CPP) {
            m_ExecutablePath = source_code.parent_path() / "main";
            m_CompilerPtr = std::make_unique<CppCompiler>();
            m_ActuatorPtr = std::make_unique<CppActuator>();
            if (!m_CompilerPtr->compile(source_code.c_str(), m_ExecutablePath.c_str(), {"-std=c++20", "-O2"}))
                this->m_CompileError = m_CompilerPtr->getCompileMessage();
        } else if (language_id == LangID::PYTHON) {
            m_ActuatorPtr = std::make_unique<PythonActuator>();
        } else {
            throw std::runtime_error{ "Unknown or unsupported Language." };
        }
    }

    std::optional<std::string> Judger::getCompileError() const
    {
        if (!m_CompileError.empty())
            return this->m_CompileError;
        return std::nullopt;
    }

    TestResult Judger::judge(const std::string& stdin_data, const std::string& expected_output)
    {
        TestResult result{};
        if (!m_CompilerPtr && getCompileError().has_value()) {
            result.status = TestStatus::COMPILATION_ERROR;
            return result;
        }

        ExecutionResult execution_result{ m_ActuatorPtr->execute(m_ExecutablePath.c_str(), m_LimitsInfo, stdin_data)};
        result = std::move(execution_result.test_result);

        if (result.status == TestStatus::UNKNOWN) {
            if (execution_result.stdout.back() == '\n')
                execution_result.stdout.pop_back();
            if (execution_result.stdout == expected_output)
                result.status = TestStatus::ACCEPTED;
            else
                result.status = TestStatus::WRONG_ANSWER;
        }
        return result;
    }
}