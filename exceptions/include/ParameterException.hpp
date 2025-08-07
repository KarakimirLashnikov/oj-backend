#pragma once
#include "Core.hpp"

namespace Exceptions
{
    class ParameterException : public std::invalid_argument
    {
    public:
        // 异常类型枚举
        enum class ExceptionType
        {
            MISSING_PARAM, // 参数缺失
            OUT_OF_RANGE,  // 值不在可选范围
            EMPTY_PARAM,   // 参数不应为空
            VALUE_ERROR,
            OTHER_ERROR    // 其他参数错误
        };

        // 构造函数
        ParameterException(
            ExceptionType type,
            const std::string &paramName,
            const std::string &extraInfo = "");

        // 获取异常类型
        inline ExceptionType getType() const noexcept { return m_type; }

        // 获取参数名称
        inline const std::string &getParamName() const noexcept { return m_paramName; }

        // 获取额外信息
        inline const std::string &getExtraInfo() const noexcept { return m_extraInfo; }

    private:
        // 生成错误信息
        static std::string generateMessage(
            ExceptionType type,
            const std::string &paramName,
            const std::string &extraInfo);

    private:
        ExceptionType m_type;
        std::string m_paramName;
        std::string m_extraInfo;
    };
}