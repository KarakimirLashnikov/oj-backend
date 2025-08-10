#include <nlohmann/json.hpp>
#include "api/problems.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "Application.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"

namespace OJApp::Problems
{
    using Core::Types::DifficultyLevel;
    using Core::Http::OK;
    using Core::Http::BadRequest;
    using Core::Http::InternalServerError;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;

    void create(const httplib::Request &req, httplib::Response &res)
    {

    }

    void update(const httplib::Request &req, httplib::Response &res)
    {

    }

    using Core::Types::TestCase;
    void uploadTestCases(const httplib::Request &req, httplib::Response &res)
    {

    }
}
