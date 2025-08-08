#include "SystemException.hpp"

namespace Exceptions
{
    Exceptions::SystemException::SystemException(int errorCode
        , const std::string &operation) noexcept
        : std::runtime_error("SystemException"),
          m_errorCode(errorCode),
          m_operation(operation),
          m_errorString(std::strerror(errorCode))
    {
        // 在构造函数中预生成what()消息
        m_whatCache = "SystemException: [" + std::to_string(m_errorCode) + "] " +
                       m_errorString + 
                       (m_operation.empty() ? "" : " - Operation: " + m_operation);
    }

    const char *SystemException::what() const noexcept
    {
        return m_whatCache.c_str();
    }

    SystemException makeSystemException(const std::string& operation)
    {
        return SystemException(errno, operation);
    }
}