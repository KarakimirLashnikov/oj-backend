#pragma once
#include <httplib.h>
#include "Logger.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"
#include "services/ServiceInfo.hpp"

namespace OJApp
{
    using namespace Core::Http;
    using Exceptions::ParameterException;
    using Exceptions::SystemException;

    using ServiceHandler = std::function<void(httplib::Response &res, njson params)>;

    void handleHttpWrapper(const httplib::Request &req, httplib::Response &res, std::initializer_list<const char*> require_params, ServiceHandler f);
}