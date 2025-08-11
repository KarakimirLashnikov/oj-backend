#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "Core.hpp"

namespace Core
{
    class Logger
    {
    public:
        // 日志级别
        enum class Level
        {
            TRACE = spdlog::level::trace,
            DEBUG = spdlog::level::debug,
            INFO = spdlog::level::info,
            WARN = spdlog::level::warn,
            ERROR = spdlog::level::err,
            CRITICAL = spdlog::level::critical,
            OFF = spdlog::level::off
        };

        /**
         * 初始化日志器
         * @param logName 日志器名称
         * @param level 日志级别
         * @param logFile 日志文件路径
         * @param async 是否使用异步日志
         * @param queueSize 异步队列大小
         * @param threadCount 异步线程数量
         * @throws std::runtime_error 如果日志器已初始化
         * @throws std::invalid_argument 如果日志文件路径不合法
         * @throws std::out_of_range 如果日志级别不在有效范围内
         */
        static void Init(const std::string &logName = "default_logger",
                         Level level = Level::INFO,
                         const std::string &logFile = "",
                         bool async = false,
                         size_t queueSize = 8192,
                         size_t threadCount = 1);

        // 获取日志器实例
        static std::shared_ptr<spdlog::logger> GetLogger();

        // 设置日志级别
        static void SetLevel(Level level);

        // 刷新日志
        static void Flush();

        // 关闭日志器
        static void Shutdown();

        // 日志记录方法
        template <typename... Args>
        static void Trace(const char *fmt, const Args &...args)
        {
            GetLogger()->trace(fmt::runtime(fmt), args...);
        }

        template <typename... Args>
        static void Debug(const char *fmt, const Args &...args)
        {
            GetLogger()->debug(fmt::runtime(fmt), args...);
        }

        template <typename... Args>
        static void Info(const char *fmt, const Args &...args)
        {
            GetLogger()->info(fmt::runtime(fmt), args...);
        }

        template <typename... Args>
        static void Warn(const char *fmt, const Args &...args)
        {
            GetLogger()->warn(fmt::runtime(fmt), args...);
        }

        template <typename... Args>
        static void Error(const char *fmt, const Args &...args)
        {
            GetLogger()->error(fmt::runtime(fmt), args...);
        }

        template <typename... Args>
        static void Critical(const char *fmt, const Args &...args)
        {
            GetLogger()->critical(fmt::runtime(fmt), args...);
        }

    private:
        static std::shared_ptr<spdlog::logger> s_logger;
        static bool s_async;
    };
}

#ifndef _RELEASE
#define LOG_TRACE(fmt, ...) \
    Core::Logger::Trace(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    Core::Logger::Debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    Core::Logger::Info(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    Core::Logger::Warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    Core::Logger::Error(fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) \
    Core::Logger::Critical(fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_WARN(fmt, ...)
#define LOG_ERROR(fmt, ...)
#define LOG_CRITICAL(fmt, ...)
#endif