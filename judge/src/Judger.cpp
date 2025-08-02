#include "Judger.hpp"
#include "Configurator.hpp"
#include "compilers/CppCompiler.hpp"
#include "actuators/CppActuator.hpp"

namespace Judge
{
    JudgeResult Judger::judge(Submission &&submission)
    {
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
            auto compiler{ this->getCompiler(submission.language_id ) };
            auto actuator{ this->getActuator(submission.language_id) };
            if (!compiler || !actuator) {
                throw std::runtime_error{ "Can't find a valid compiler/actuator for this language." };
            }
            std::string exe_dir{ "SubmissionExeDir/" + std::to_string(submission.submission_id) };
            if (fs::create_directories(exe_dir)) {
                if (compiler->compile(submission.source_code, exe_dir + "/main", submission.compile_options)) {
                    result.setCompileMessage( compiler->getCompileMessage() );
                    std::vector<std::future<TestResult>> fut_list;
                    for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                        TestResult test_result{};
                        test_result.index = i;
                        const auto& [input_data, expected_output] = submission.test_cases[i];
                        fut_list.emplace_back(std::async(std::launch::async, [&](){
                            test_result.setResult(actuator->execute(exe_dir + "/main", limits, input_data));
                            return test_result;
                        } ));
                    }
                    for (size_t i{0}; i < fut_list.size(); ++i) {
                        const auto& [input_data, expected_output] = submission.test_cases[i];
                        result.insertTestResult( this->judgeTest(expected_output, fut_list[i].get()) );
                    }
                    // 删除可执行文件
                    fs::remove_all(exe_dir);
                } else {
                    result.setCompileMessage("Compile Error: " + compiler->getCompileMessage());
                    for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                        TestResult test_result{};
                        test_result.index = i;
                        test_result.status = TestStatus::COMPILATION_ERROR;
                        result.insertTestResult(std::move(test_result));
                    }
                    return result;
                }
            } else {
                throw std::runtime_error{ "Failed to create exe directory: " + exe_dir };
            }
        }
        catch (std::exception& e) {
            LOG_ERROR("An error occur in judger: {}", e.what());
            for (size_t i{0}; i < submission.test_cases.size(); ++i) {
                TestResult test_result{};
                test_result.index = i;
                test_result.status = TestStatus::INTERNAL_ERROR;
                result.insertTestResult(std::move(test_result));
            }
            return result;
        }
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

    TestResult Judger::judgeTest(const ExpectedOutput &expected_output, TestResult &&result)
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