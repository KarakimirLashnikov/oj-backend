#pragma once
#include "Core.hpp"
#include "Application.hpp"


namespace OJApp::API
{
    void dealSubmit(const httplib::Request& req, httplib::Response& res);
}