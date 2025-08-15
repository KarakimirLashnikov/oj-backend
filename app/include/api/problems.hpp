#pragma once

#include <httplib.h>
#include "Core.hpp"

namespace OJApp::Problems
{
    void createProblem(const httplib::Request& req, httplib::Response& res);

    void addProblemLimit(const httplib::Request& req, httplib::Response& res);

    void uploadTestCases(const httplib::Request& req, httplib::Response& res);

    void getProblemList(const httplib::Request& req, httplib::Response& res);
}