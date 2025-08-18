#include "api/submissions.hpp"
#include "api/api.hpp"
#include "services/SubmissionService.hpp"
#include "Application.hpp"

namespace OJApp::Submissions
{
    using Core::Types::SubmissionInfo;
    using Core::Types::LangID;

    void submit(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "token", "username", "problem_title", "source_code", "language_id" },
            [&](httplib::Response& res, njson request) -> void {
                SubmissionInfo info{};
                info.username = request.at("username").get<std::string>();
                info.problem_title = request.at("problem_title").get<std::string>();
                info.source_code = request.at("source_code").get<std::string>();
                info.language_id = static_cast<LangID>(request.at("language_id").get<int>());

                const std::string token{ request.at("token").get<std::string>() };

                ServiceInfo sv_info = App.getSubmissionService().submit(std::move(info), token);

                res.status = sv_info.status;
                njson response = njson{ {"message", sv_info.message} };
                res.set_content(response.dump(), "application/json");
            }
        );
    }

    void querySubmissions(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "token", "submission_id" },
            [&](httplib::Response& res, njson request) -> void {
                const std::string token{ request.at("token").get<std::string>() };
                const std::string submission_id{ request.at("submission_id").get<std::string>() };

                ServiceInfo sv_info = App.getSubmissionService().querySubmission(submission_id, token);

                res.status = sv_info.status;
                njson response = njson{ {"message", sv_info.message} };
                res.set_content(response.dump(), "application/json");
            }
        );
    }
}

