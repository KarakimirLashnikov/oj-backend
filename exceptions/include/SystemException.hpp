#pragma once
#include "Core.hpp"

namespace Exceptions
{
    class SystemException : public std::runtime_error
    {
    public:
        // 构造函数
        explicit SystemException(int errorCode
            , const std::string &operation
            , const char* file
            , int line) noexcept;
        
        // 获取系统错误码
        inline int errorCode() const noexcept { return m_errorCode; }
        
        // 获取相关操作描述
        inline const std::string &operation() const noexcept { return m_operation; }
        
        // 获取错误描述字符串
        inline const char *errorString() const noexcept { return m_errorString.c_str(); }
        
        // 重写what()以提供更详细的信息
        const char *what() const noexcept override;

    private:
        int m_errorCode;
        std::string m_operation;
        std::string m_errorString;
        mutable std::string m_whatCache; // 缓存what()结果
    };

    // 根据当前errno创建系统异常
    SystemException makeSystemException(const std::string& operation, const char* file, int line);
}