#pragma once
#include "Http.hpp"
#include "Types.hpp"

namespace OJApp
{
    using namespace Core::Http;

    struct ServiceInfo 
    {
        njson message;
        ResponseStatusCode status;

        inline operator bool() const { return isSuccess(this->status); }
    };
}