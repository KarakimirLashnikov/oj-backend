#include <nlohmann/json.hpp>
#include "api/problems.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "utilities/Problem.hpp"
#include "Application.hpp"
#include "judgedb/ProblemWriter.hpp"
#include "judgedb/ProblemInquirer.hpp"
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
        try {
            res.status = OK;
            auto response = njson{{"status", "success"}, {"message", problem.title + " created successfully"}};
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const sql::SQLException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }

    void update(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson json = njson::parse(req.body);
            if (!json.contains("title") || 
                !json.contains("time_limit_s") || 
                !json.contains("memory_limit_kb") || 
                !json.contains("stack_limit_kb") || 
                !json.contains("difficulty") || 
                !json.contains("description")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "problem", "Missing parameters");
            }
            Judge::ResourceLimits limits{
                .time_limit_s = json.at("time_limit_s").get<float>(),
                .extra_time_s = json.contains("extra_time_s") ? json.at("extra_time_s").get<float>() : 0.5f,
                .wall_time_s = json.contains("wall_time_s") ? json.at("wall_time_s").get<float>() : json.at("time_limit_s").get<float>() + 1.0f,
                .memory_limit_kb = json.at("memory_limit_kb").get<uint32_t>(),
                .stack_limit_kb = json.at("stack_limit_kb").get<uint32_t>()
            };

            Judge::Problem problem{
                .title = json.at("title").get<std::string>(),
                .description = json.at("description").get<std::string>(),
                .level = static_cast<Core::Types::DifficultyLevel>(json.at("difficulty").get<int>()),
                .limits = std::move(limits)
            };
            
            auto& cfg{ App.getConfigurator() };
            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD"),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            if (!pi.isExist(problem.title)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "title", "Problem " + problem.title + " does not exist");
            }

            JudgeDB::ProblemWriter pw{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD"),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            pw.updateProblem(problem);

            res.status = OK;
            auto response = njson{{"status", "success"}, {"message", problem.title + " updated successfully"}};
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const sql::SQLException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }

    using Core::Types::TestCase;
    void uploadTestCases(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson json = njson::parse(req.body);
            if (!json.contains("problem_title") || 
                !json.contains("test_cases")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "problem", "Missing parameters");
            }
            std::string title{ json.at("problem_title").get<std::string>() };

            auto& cfg{ App.getConfigurator() };
            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "HOST", "127.0.0.1"),
                cfg.get<uint16_t>("judgedb", "PORT", 3306),
                cfg.get<std::string>("judgedb", "USERNAME", "root"),
                cfg.get<std::string>("judgedb", "PASSWORD"),
                cfg.get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            if (!pi.isExist(title)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "title", "Problem " + title + " does not exist");
            }

            std::vector<TestCase> cases{};
            for (auto& elem : json.at("test_cases")) {
                try {
                    TestCase tc{
                        .stdin = elem.at("stdin").get<std::string>(),
                        .expected_output = elem.at("expected_output").get<std::string>(),
                        .sequence = elem.at("sequence").get<int>(),
                        .is_hidden = elem.contains("hidden") ? elem.at("hidden").get<bool>() : false
                    };
                    cases.push_back(tc);
                } catch (const std::out_of_range& e) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "one of test case fields", "Missing test cases");
                }
            }

            if (!App.uploadTestCases(std::move(cases), title)) {
                throw Exceptions::makeSystemException("upload test cases failed");
            }
            res.status = OK;
            auto response = njson{{"status", "success"}, {"message", "Test cases successfully uploaded for " + title}};
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const sql::SQLException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const Exceptions::SystemException& e) {
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = BadRequest;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }
}
