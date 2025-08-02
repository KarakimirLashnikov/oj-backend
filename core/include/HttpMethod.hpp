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
}