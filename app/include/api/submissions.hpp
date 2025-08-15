#pragma once
#include <httplib.h>
#include "Core.hpp"


namespace OJApp::Submissions
{
    void submit(const httplib::Request& req, httplib::Response& res);

    void querySubmissions(const httplib::Request& req, httplib::Response& res);
}