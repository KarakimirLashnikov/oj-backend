#pragma once
#include "Core.hpp"

namespace Core::Types
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    using Millis = std::chrono::milliseconds;
    using SubID = boost::uuids::random_generator_pure::result_type;
    using StatusCode = std::uint8_t;

    template <typename T>
    using Queue = boost::lockfree::queue<T>;
}

namespace bp = boost::process;
namespace fs = std::filesystem;
namespace asio = boost::asio;