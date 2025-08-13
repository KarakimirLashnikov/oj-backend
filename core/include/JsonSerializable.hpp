#pragma once
#include "Core.hpp"

using njson = nlohmann::json;

namespace Core
{
    
    class JsonSerializable
    {
    public:
        virtual ~JsonSerializable() = default;

        // 序列化：对象 → JSON
        virtual njson toJson() const = 0;

        // 反序列化：JSON → 对象
        virtual void fromJson(const njson &j) = 0;

        // 直接返回 JSON 字符串
        inline std::string toJsonString() const
        {
            return toJson().dump(); // 默认紧凑格式（无缩进）
        }

        // 从 JSON 字符串初始化
        inline void fromJsonString(const std::string &jsonStr)
        {
            fromJson(njson::parse(jsonStr));
        }
    };
}