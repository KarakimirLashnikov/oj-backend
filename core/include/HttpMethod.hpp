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

    template <typename T>
    concept HttpMethod = std::is_same<T, GET>::value
        || std::is_same<T, POST>::value
        || std::is_same<T, PUT>::value
        || std::is_same<T, DELETE>::value
        || std::is_same<T, PATCH>::value
        || std::is_same<T, HEAD>::value
        || std::is_same<T, OPTIONS>::value
        || std::is_same<T, TRACE>::value
        || std::is_same<T, CONNECT>::value;
}