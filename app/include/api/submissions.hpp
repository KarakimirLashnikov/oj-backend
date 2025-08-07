#pragma once
#include "Core.hpp"
#include <httplib.h>


namespace OJApp::Submissions
{
    void submit(const httplib::Request& req, httplib::Response& res);
}