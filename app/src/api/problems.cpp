#include "api/problems.hpp"
#include "api/api.hpp"
#include "services/ProblemService.hpp"

namespace OJApp::Problems
{
    using Core::Types::DifficultyLevel;
    using Core::Types::ProblemInfo;
    using Core::Types::ProblemLimitsInfo;
    using Core::Types::TestCaseInfo;

    void createProblem(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "token", "username", "title", "difficulty", "description" }, 
            [&](httplib::Response& res, njson request) -> void{
                ProblemInfo info{};
                info.fromJson(request);
                
                const std::string token{ request.at("token").get<std::string>() };

                ProblemService service;
                ServiceInfo sv_info = service.createProblem(std::move(info), token);
                
                res.status = sv_info.status;
                njson response = njson{ { "message", sv_info.message } };
                res.set_content(response.dump(), "application/json");
            }
        );
    }

    void addProblemLimit(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "token", "problem_title", "language_id", "time_limit_s", "extra_time_s", "wall_time_s", "memory_limit_kb", "stack_limit_kb" }, 
            [&](httplib::Response& res, njson request) -> void{
                ProblemLimitsInfo info{};
                info.fromJson(request);
                
                const std::string token{ request.at("token").get<std::string>() };

                ProblemService service;
                ServiceInfo sv_info = service.addProblemLimit(std::move(info), token);
                
                res.status = sv_info.status;
                njson response = njson{ { "message", sv_info.message } };
                res.set_content(response.dump(), "application/json");
            }
        );
    }

    void uploadTestCases(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "token", "problem_title", "stdin_data", "expected_output", "sequence" },
            [&](httplib::Response& res, njson request) -> void {
                TestCaseInfo info{};
                info.fromJson(request);

                const std::string token{ request.at("token").get<std::string>() };

                ProblemService service;
                ServiceInfo sv_info = service.uploadTestCases(std::move(info), token);

                res.status = sv_info.status;
                njson response = njson{ {"message", sv_info.message} };
                res.set_content(response.dump(), "application/json");
            }
        );
    }
}