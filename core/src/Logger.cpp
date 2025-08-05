#include "Logger.hpp"

namespace Core
{
    std::shared_ptr<spdlog::logger> Logger::s_logger{nullptr};
    bool Logger::s_async{false};

    void Logger::Init(const std::string &logName, Level level,
                      const std::string &logFile, bool async,
                      size_t queueSize, size_t threadCount)
    {
        if (s_logger)
        {
            throw std::runtime_error("Logger already initialized");
        }

        s_async = async;

        if (async)
        {
            static std::once_flag onceFlag;
            std::call_once(onceFlag, [&]()
                           { spdlog::init_thread_pool(queueSize, threadCount); });
        }

        std::vector<spdlog::sink_ptr> sinks;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        sinks.push_back(console_sink);

        if (!logFile.empty())
        {
            if (logFile.find("..") != std::string::npos) {
                throw std::invalid_argument("Invalid log file path");
            }
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
            sinks.push_back(file_sink);
        }

        if (async)
        {
            s_logger = std::make_shared<spdlog::async_logger>(
                logName, sinks.begin(), sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
        }
        else
        {
            s_logger = std::make_shared<spdlog::logger>(logName, sinks.begin(), sinks.end());
        }

        // 修复3：添加枚举范围检查
        if (level < Level::TRACE || level > Level::OFF)
        {
            throw std::out_of_range("Invalid log level");
        }
        SetLevel(level);

        s_logger->flush_on(static_cast<spdlog::level::level_enum>(Level::WARN));
        spdlog::register_logger(s_logger);
    }

    std::shared_ptr<spdlog::logger> Logger::GetLogger()
    {
        assert(s_logger != nullptr && "Logger not initialized");
        return s_logger;
    }

    void Logger::SetLevel(Level level)
    {
        // 双重检查确保级别有效
        if (level < Level::TRACE || level > Level::OFF)
        {
            throw std::out_of_range("Invalid log level");
        }
        GetLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
    }

    void Logger::Flush()
    {
        GetLogger()->flush();
    }

    void Logger::Shutdown()
    {
        if (s_logger && s_async)
        {
            GetLogger()->set_level(spdlog::level::off); // Disable logging
            spdlog::shutdown(); // Shutdown the logger
            s_logger.reset(); // Clear the logger instance
        }
    }
}