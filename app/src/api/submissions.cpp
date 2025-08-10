#include <nlohmann/json.hpp>
#include "Application.hpp"
#include "api/submissions.hpp"
#include "utilities/Submission.hpp"
#include "utilities/Language.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"

namespace OJApp::Submissions
{
    using Judge::Submission;
    using Core::Types::SubID;
    using Judge::LangID;
    using Core::Http::InternalServerError;
    using Core::Http::BadRequest;
    using Core::Http::OK;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;

    void submit(const httplib::Request &req, httplib::Response &res)
    {

    }
}