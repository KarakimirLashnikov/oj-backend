#include "Types.hpp"

namespace Core::Types
{
    std::string timeStampToMySQLString(const TimeStamp& ts) 
    {
        // 锚点：程序启动时，同时记录两个时钟的时间点（仅初始化一次）
        static const struct Anchor {
            std::chrono::steady_clock::time_point steady;  // steady_clock的锚点
            std::chrono::system_clock::time_point system;   // system_clock的对应锚点
        } anchor = {
            std::chrono::steady_clock::now(),
            std::chrono::system_clock::now()
        };

        // 计算ts相对于steady锚点的偏移（duration类型）
        auto offset = ts - anchor.steady;

        // 将偏移加到system锚点上，得到对应的system_clock时间点
        auto system_ts = anchor.system + offset;

        // 转换为Unix时间戳（秒）
        auto unix_ts = std::chrono::system_clock::to_time_t(system_ts);

        // 转换为UTC时间的tm结构体
        std::tm tm = *std::gmtime(&unix_ts);  // 使用gmtime避免时区问题

        // 格式化为MySQL TIMESTAMP字符串（YYYY-MM-DD HH:MM:SS）
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}