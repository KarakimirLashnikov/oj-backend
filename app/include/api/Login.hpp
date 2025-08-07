#pragma once
#include <httplib.h>
#include "Core.hpp"

namespace OJApp::Login
{
    // 应该使用 https

    void login(const httplib::Request &req, httplib::Response &res);

    void signup(const httplib::Request &req, httplib::Response &res);
}