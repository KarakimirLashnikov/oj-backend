#include "Logger.hpp"
#include "Judger.hpp"

int main()
{
    try
    {
        Core::Logger::Init("core", Core::Logger::Level::INFO, "logs/judge.log");
        Judge::Judger judger("http://47.102.132.177:2358/");
        httplib::Params params{
            {"language_id", "54"},
            {"source_code", "#include <stdio.h>\n int main() { printf(\"hello world\"); return 0; }"},
            {"stdin", ""},
            {"expected_output", "hello world"},
            {"cpu_time_limit", "1"},
            {"memory_limit", "64000"}};
        auto token{judger.submit(params)};
        LOG_INFO("Token: {}", token);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto result{judger.getJudgeResult("3359d179-ccf2-4705-9d3b-7b936ddb72a7")};
        LOG_INFO("Result\n: {}", result.dump());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}