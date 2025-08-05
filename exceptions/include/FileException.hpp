#pragma once
#include "Core.hpp"
#include "Types.hpp"

namespace Exceptions
{
    class FileException : public std::runtime_error
    {
    public:
        enum class ErrorType
        {
            FileNotFound,     // 文件不存在
            PermissionDenied, // 没有权限
            OpenFailed,       // 打开失败
            ReadError,        // 读取错误
            WriteError,       // 写入错误
            SeekError,        // 定位错误
            AlreadyExists,    // 文件已存在
            InvalidOperation, // 无效操作
            UnknownError      // 未知错误
        };

        // 构造函数
        FileException(ErrorType type, const std::string &path, const std::string &message);

        FileException(ErrorType type, const std::string &path, const std::string &message, int errorCode);

        // 获取异常类型
        inline ErrorType type() const noexcept { return m_type; }

        // 获取相关文件路径
        inline const std::string &path() const noexcept { return m_path; }

        // 获取系统错误码（如果有）
        inline int errorCode() const noexcept { return m_errorCode; }

        // 获取错误类型字符串
        static const char *errorTypeToString(ErrorType type);

        // 重写what()以提供更详细的信息
        const char *what() const noexcept override;

    private:
        ErrorType m_type;
        std::string m_path;
        int m_errorCode;
        mutable std::string m_whatCache; // 缓存what()结果
    };


    // 辅助函数：根据系统错误创建适当的文件异常
    FileException makeFileException(const std::string& path, const std::string& operation = "");

    // 检查文件是否存在，不存在则抛出异常
    void checkFileExists(const std::string& path);

    // 检查文件是否可读
    void checkFileReadable(const std::string& path);

    // 检查文件是否可写
    void checkFileWritable(const std::string& path);

    // 检查是否是可执行文件
    void checkFileExecutable(const std::string &path);

    // 检查文件是否存在并可访问
    void checkFileAccessible(const std::string& path);
}