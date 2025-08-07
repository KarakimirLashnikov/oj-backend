#pragma once

#include "Core.hpp"
#include <httplib.h>

namespace OJApp::Problems
{
    void create(const httplib::Request& req, httplib::Response& res);

    void update(const httplib::Request& req, httplib::Response& res);

    void uploadTestCases(const httplib::Request& req, httplib::Response& res);
}