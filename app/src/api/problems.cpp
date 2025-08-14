#include "api/problems.hpp"
#include "api/api.hpp"
#include "services/ProblemService.hpp"

namespace OJApp::Problems
{
    using Core::Types::DifficultyLevel;
    using Core::Types::ProblemInfo;

    void createProblem(const httplib::Request &req, httplib::Response &res)
    {
        handleHttpWrapper(req, res, { "username", "password", "email" }, 
            [&](httplib::Response& res, njson request) -> void{
                ProblemInfo info{};
                info.title = request.at("title").get<std::string>();
                info.difficulty = static_cast<DifficultyLevel>(request.at("difficulty").get<unsigned int>());
                info.description = request.at("description").get<std::string>();
                
                const std::string token{ request.at("token").get<std::string>() };

                ProblemService service;
                ServiceInfo sv_info = service.createProblem(std::move(info), token);
                
                res.status = sv_info.status;
                njson response = { { "message", sv_info.message } };
                res.set_content(response.dump(), "application/json");
            }
        );
    }

    void addProblemLimit(const httplib::Request &req, httplib::Response &res)
    {
    }

    void uploadTestCases(const httplib::Request &req, httplib::Response &res)
    {
    }
}