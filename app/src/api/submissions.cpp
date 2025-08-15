#include "api/submissions.hpp"
#include "api/api.hpp"
#include "services/SubmissionService.hpp"

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

                SubmissionService service;
                ServiceInfo sv_info = service.submit(std::move(info), token);

                res.status = sv_info.status;
                njson response = njson{ {"message", sv_info.message} };
                res.set_content(response.dump(), "application/json");
            }
        );
    }
}
