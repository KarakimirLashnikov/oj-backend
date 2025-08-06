#pragma once
#include "Core.hpp"

namespace Core::Types
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    using Millis = std::chrono::milliseconds;
    using SubID = boost::uuids::random_generator_pure::result_type;
    using StatusCode = std::uint8_t;

    struct TestCase
    {
        std::string stdin;
        std::string expected_output;
        int sequence;
        bool is_hidden = false;
    };

    template <typename T>
    concept IniSupportedType = std::is_integral_v<T> || 
                            std::is_same_v<T, std::string> ||
                            std::is_floating_point_v<T> ||
                            std::is_same_v<T, bool>;

    std::string timeStampToMySQLString(const TimeStamp& ts);
}

namespace bp = boost::process;
namespace fs = std::filesystem;
namespace asio = boost::asio;