#include "ParameterException.hpp"

namespace Exceptions
{
    ParameterException::ParameterException(
        ExceptionType type,
        const std::string &paramName,
        const std::string &extraInfo)
        : std::invalid_argument(generateMessage(type, paramName, extraInfo)),
          m_type(type),
          m_paramName(paramName),
          m_extraInfo(extraInfo)
    {
    }

    std::string ParameterException::generateMessage(
        ExceptionType type,
        const std::string &paramName,
        const std::string &extraInfo)
    {
        switch (type)
        {
        case ExceptionType::MISSING_PARAM:
            return "Missing required parameter: " + paramName;
        case ExceptionType::OUT_OF_RANGE:
            return "Parameter out of range: " + paramName +
                   (extraInfo.empty() ? "" : " (" + extraInfo + ")");
        case ExceptionType::EMPTY_PARAM:
            return "Parameter cannot be empty: " + paramName;
        case ExceptionType::OTHER_ERROR:
            return "Parameter error: " + paramName +
                   (extraInfo.empty() ? "" : " - " + extraInfo);
        case ExceptionType::VALUE_ERROR:
            return "Invalid parameter value: " + paramName;
        default:
            return "Unknown parameter error: " + paramName;
        }
    }

    void checkJsonParamExist(const njson &obj, std::initializer_list<const char*> params)
    {
        for (auto it : params) 
        {
            if (!obj.contains(it)) {
                throw ParameterException(ParameterException::ExceptionType::MISSING_PARAM, it);
            }
        }
    }
}