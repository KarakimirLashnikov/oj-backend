#pragma once
#include "Core.hpp"

namespace Core::Types
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    using Millis = std::chrono::milliseconds;
    using SubID = boost::uuids::random_generator_pure::result_type;
    using StatusCode = std::uint8_t;
    using TestInput = std::string;
    using ExpectedOutput = std::string;
    using TestCase = std::pair<TestInput, ExpectedOutput>;
    using TestCases = std::vector<TestCase>;
    template <typename T> using Queue = boost::lockfree::queue<T>;
}

namespace bp = boost::process;
namespace fs = std::filesystem;
namespace asio = boost::asio;