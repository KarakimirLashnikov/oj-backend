#pragma once
#include <httplib.h>
#include "Core.hpp"

namespace OJApp::Login
{
    void login(const httplib::Request &req, httplib::Response &res);

    void signup(const httplib::Request &req, httplib::Response &res);
}