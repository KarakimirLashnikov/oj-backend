#pragma once

#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/extend.hpp>


#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <thread>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include <concepts>
#include <optional>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <future>
#include <condition_variable>
#include <queue>
#include <functional>
#include <ranges>
#include <numeric>
#include <tuple>
#include <format>
#include <atomic>

#define BITMASK(x) (1ULL << (x))