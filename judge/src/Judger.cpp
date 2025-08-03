#include "Judger.hpp"
#include "Configurator.hpp"
#include "compilers/CppCompiler.hpp"
#include "actuators/CppActuator.hpp"

namespace Judge
{
    JudgeResult Judger::judge(Submission submission)
    {
        LOG_INFO("Judger Judging: {}", __FILE__);
        JudgeResult result{submission.problem, submission.submission_id, submission.test_cases.size()};
        result.setCreateTime(TimeStamp::clock::now());

        ResourceLimits limits{
            .cpu_time_limit_s = submission.cpu_time_limit_s,
            .cpu_extra_time_s = submission.cpu_extra_time_s,
            .wall_time_limit_s = submission.wall_time_limit_s,
            .memory_limit_kb = submission.memory_limit_kb,
            .stack_limit_kb = submission.stack_limit_kb
        };

        try {
            // 获取源码所在目录作为工作目录
            auto work_dir = std::filesystem::path(submission.source_code_path).parent_path();
            if (work_dir.empty()) {
                LOG_ERROR("Can't find source file {} directory", submission.source_code_path.data());
                throw std::system_error(errno, std::system_category(), "Get directory failed");
            }
            auto exe_path = work_dir / "main";  // 可执行文件路径
            
            auto compiler{ this->getCompiler(submission.language_id ) };
            auto actuator{ this->getActuator(submission.language_id) };
            
            if (!compiler || !actuator) {
                throw std::runtime_error{ "Can't find a valid compiler/actuator for this language." };
            }
            
            if (compiler->compile(submission.source_code_path, exe_path.string(), submission.compile_options)) {
                result.setCompileMessage( compiler->getCompileMessage() );
                std::vector<std::future<TestResult>> fut_list;
                
                // 异步执行所有测试用例
                for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                    const auto& [input_data, expected_output] = submission.test_cases[i];
                    fut_list.emplace_back(std::async(std::launch::async, [&, i, input_data = input_data](){
                        TestResult test_result{};
                        test_result.index = i;
                        test_result.setResult(actuator->execute(exe_path.string(), limits, input_data));
                        return test_result;
                    }));
                }
                
                // 收集所有测试结果
                for (size_t i{0}; i < fut_list.size(); ++i) {
                    const auto& [input_data, expected_output] = submission.test_cases[i];
                    result.insertTestResult(this->judgeTest(expected_output, fut_list[i].get()));
                }
            } else {
                result.setCompileMessage("Compile Error: " + compiler->getCompileMessage());
                for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                    TestResult test_result{};
                    test_result.index = i;
                    test_result.status = TestStatus::COMPILATION_ERROR;
                    result.insertTestResult(std::move(test_result));
                }
            }
        } catch (std::exception& e) {
            LOG_ERROR("An error occurred in judger: {}", e.what());
            for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                TestResult test_result{};
                test_result.index = i;
                test_result.status = TestStatus::INTERNAL_ERROR;
                result.insertTestResult(std::move(test_result));
            }
            result.setCompileMessage("Error: " + std::string(e.what()));
            throw e;
        }
        LOG_INFO("Judger Judged: {}", result.toString());
        return result;
    }

    std::unique_ptr<Compiler> Judger::getCompiler(LangID lang_id)
    {
        switch (lang_id)
        {
        case LangID::CPP: return createCompiler<CppCompiler>();
        default: return nullptr;
        }
    }

    std::unique_ptr<Actuator> Judger::getActuator(LangID lang_id)
    {
        switch (lang_id)
        {
        case LangID::CPP: return createActuator<CppActuator>();
        default: return nullptr;
        }
    }

    TestResult Judger::judgeTest(const ExpectedOutput &expected_output, TestResult result)
    {
        if (result.status == TestStatus::UNKNOWN) {
            if (result.stdout == expected_output) {
                result.status = TestStatus::ACCEPTED;
            }
            else {
                result.status = TestStatus::WRONG_ANSWER;
            }
        }
        return result;
    }
}