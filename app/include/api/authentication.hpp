#pragma once
#include <httplib.h>
#include "Core.hpp"

namespace OJApp::Authentication
{
    void registry(const httplib::Request &req, httplib::Response &res);
    
    void login(const httplib::Request &req, httplib::Response &res);
}