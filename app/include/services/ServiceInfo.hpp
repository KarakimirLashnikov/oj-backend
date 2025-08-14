#pragma once
#include "Http.hpp"

namespace OJApp
{
    using namespace Core::Http;

    struct ServiceInfo 
    {
        std::string message;
        ResponseStatusCode status;

        inline operator bool() const { return isSuccess(this->status); }
    };
}