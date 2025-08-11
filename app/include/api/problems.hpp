#pragma once

#include "Core.hpp"
#include <httplib.h>

namespace OJApp::Problems
{
    void createProblem(const httplib::Request& req, httplib::Response& res);

    void addProblemLimit(const httplib::Request& req, httplib::Response& res);

    void uploadTestCases(const httplib::Request& req, httplib::Response& res);
}