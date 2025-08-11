#include <nlohmann/json.hpp>
#include "Application.hpp"
#include "api/submissions.hpp"
#include "utilities/Submission.hpp"
#include "utilities/Language.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"
#include "AuthService.hpp"

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
    using Exceptions::SystemException;
    using Exceptions::ParameterException;

    void submit(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson j = njson::parse(req.body);
            Exceptions::checkJsonParamExist(j, {"token", "problem_id", "code", "language"});
            AuthService& auth_service{ App.getAuthService() };
            auto user_info{ auth_service.getCurrentUser(j.at("token").get<std::string>()) };
            if (!user_info)
                throw ParameterException(VALUE_ERROR, "token");

            std::string sub_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
            Submission submission{
                .user_uuid = user_info.value().user_uuid,
                .problem_title = j.at("problem_id").get<std::string>(),
                .source_code = j.at("code").get<std::string>(),
                .submission_id = sub_uuid,
                .language_id = static_cast<LangID>(j.at("language").get<int>())
            };

            if (!App.submit(std::move(submission))) {
                res.status = InternalServerError;
                njson res_content = njson{{"status", "failed"}, {"message", "save source code failed."}};
                res.set_content(res_content.dump(), "application/json");
                return;
            }
            res.status = OK;
            njson res_content = njson{{"status", "success"}, {"message", "submit succeed"}, {"submission_id", submission.submission_id}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const ParameterException& e) {
            LOG_INFO("CreateRequest ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failure"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const SystemException& e) {
            LOG_INFO(e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failure"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }
}