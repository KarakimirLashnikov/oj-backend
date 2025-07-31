#include "Logger.hpp"
#include "Judger.hpp"
#include "SubmissionSchema.hpp"

int main()
{
    try
    {
        Judge::SubmissionSchema schema{1, R"PYTHON(print("Hello, World!"))PYTHON", 54};
        auto v1 = Judge::SubmissionSchema::toHttpParams(schema);

        for (auto &case_: v1)
        {
            for (auto &[key, value]: case_)
                std::cout << key << ": " << value << '\n';
            std::cout << '\n';
        }

        schema.test_cases.emplace_back("hello world", "hello world");
        schema.test_cases.emplace_back("test", "test");
        schema.test_cases.emplace_back("12345", "12345");
        auto v2 = Judge::SubmissionSchema::toHttpParams(schema);
        for (auto &case_: v2)
        {
            for (auto &[key, value]: case_)
                std::cout << key << ": " << value << '\n';
            std::cout << '\n';
        }


        // Core::Logger::Init("core", Core::Logger::Level::INFO, "logs/judge.log");
        // Judge::Judger judger("http://47.102.132.177:2358/");
        // httplib::Params params{
        //     {"language_id", "54"},
        //     {"source_code", "#include <stdio.h>\n int main() { printf(\"hello world\"); return 0; }"},
        //     {"stdin", ""},
        //     {"expected_output", "hello world"},
        //     {"cpu_time_limit", "1"},
        //     {"memory_limit", "64000"}};
        // auto token{judger.submit(params)};
        // LOG_INFO("Token: {}", token);
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        // auto result{judger.getJudgeResult(token)};
        // LOG_INFO("Result\n: {}", result.dump());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}