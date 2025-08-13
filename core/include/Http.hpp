#pragma once

#include "Core.hpp"

namespace Core::Http
{
    struct GET{};
    struct POST{};
    struct PUT{};
    struct DELETE{};
    struct PATCH{};
    struct HEAD{};
    struct OPTIONS{};
    struct TRACE{};
    struct CONNECT{};

    template <typename TMethod>
    concept HttpMethod = std::is_same_v<TMethod, GET> ||
                         std::is_same_v<TMethod, POST> ||
                         std::is_same_v<TMethod, PUT> ||
                         std::is_same_v<TMethod, DELETE> ||
                         std::is_same_v<TMethod, PATCH> ||
                         std::is_same_v<TMethod, HEAD> ||
                         std::is_same_v<TMethod, OPTIONS> ||
                         std::is_same_v<TMethod, TRACE> ||
                         std::is_same_v<TMethod, CONNECT>;

    enum ResponseStatusCode
    {
        // 2xx Success
        OK = 200,
        Created = 201,          // 资源创建成功（如 POST 请求）
        Accepted = 202,         // 请求已接受但未处理完成
        NoContent = 204,        // 成功但无返回内容（如 DELETE 请求）

        // 3xx Redirection
        MovedPermanently = 301, // 资源永久重定向
        Found = 302,            // 资源临时重定向

        // 4xx Client Errors
        BadRequest = 400,       // 请求语法错误
        Unauthorized = 401,     // 未认证（需登录）
        Forbidden = 403,        // 无权限访问
        NotFound = 404,         // 资源不存在
        MethodNotAllowed = 405, // HTTP 方法不支持
        Conflict = 409,         // 资源冲突（如重复创建）
        TooManyRequests = 429,  // 请求过于频繁

        // 5xx Server Errors
        InternalServerError = 500, // 服务器内部错误
        NotImplemented = 501,     // 功能未实现
        BadGateway = 502,         // 网关错误
        ServiceUnavailable = 503,  // 服务不可用（系统繁忙）
        GatewayTimeout = 504       // 网关超时
    };

    inline bool isSuccess(const ResponseStatusCode code)
    {
        return code >= 200 && code < 300;
    }
    inline bool isRedirection(const ResponseStatusCode code)
    {
        return code >= 300 && code < 400;
    }
    inline bool isClientError(const ResponseStatusCode code)
    {
        return code >= 400 && code < 500;
    }
    inline bool isServerError(const ResponseStatusCode code)
    {
        return code >= 500;
    }
}