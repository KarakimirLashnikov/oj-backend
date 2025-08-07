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
            auto json{ njson::parse(req.body) };
            if (!json.contains("title") || 
                !json.contains("time_limit_s") || 
                !json.contains("memory_limit_kb") || 
                !json.contains("stack_limit_kb") || 
                !json.contains("difficulty") || 
                !json.contains("description")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "problem", "参数缺失");
            }
            Judge::Problem problem{
                .title = json.at("title").get<std::string>(),
                .time_limit_s = json.at("time_limit_s").get<float>(),
                .memory_limit_kb = json.at("memory_limit_kb").get<uint32_t>(),
                .stack_limit_kb = json.at("stack_limit_kb").get<uint32_t>(),
                .level = static_cast<Core::Types::DifficultyLevel>(json.at("difficulty").get<int>()),
                .description = json.at("description").get<std::string>()
            };

            auto& cfg{ App.getConfigurator() };
            JudgeDB::ProblemWriter pw{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };

            pw.createProblem(problem);
            res.status = OK;
            auto response{ njson{{"status", "success"}, {"message", problem.title + " 问题添加成功"}} };
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
            auto json{ njson::parse(req.body) };
            if (!json.contains("title") || 
                !json.contains("time_limit_s") || 
                !json.contains("memory_limit_kb") || 
                !json.contains("stack_limit_kb") || 
                !json.contains("difficulty") || 
                !json.contains("description")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "problem", "参数缺失");
            }
            Judge::Problem problem{
                .title = json.at("title").get<std::string>(),
                .time_limit_s = json.at("time_limit_s").get<float>(),
                .memory_limit_kb = json.at("memory_limit_kb").get<uint32_t>(),
                .stack_limit_kb = json.at("stack_limit_kb").get<uint32_t>(),
                .level = static_cast<Core::Types::DifficultyLevel>(json.at("difficulty").get<int>()),
                .description = json.at("description").get<std::string>()
            };
            
            auto& cfg{ App.getConfigurator() };
            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };
            if (!pi.isExist(problem.title)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "title", "问题不存在");
            }

            JudgeDB::ProblemWriter pw{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };
            pw.updateProblem(problem);

            res.status = OK;
            auto response{ njson{{"status", "success"}, {"message", problem.title + " 问题编辑成功"}} };
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
            auto json{ njson::parse(req.body) };
            if (!json.contains("problem_title") || 
                !json.contains("test_cases")) {
                    throw Exceptions::ParameterException(MISSING_PARAM, "problem", "参数缺失");
            }
            std::string title{ json.at("problem_title").get<std::string>() };

            auto& cfg{ App.getConfigurator() };
            JudgeDB::ProblemInquirer pi{
                cfg.get<std::string>("judgedb", "host", "127.0.0.1"),
                cfg.get<std::string>("judgedb", "username", "root"),
                cfg.get<std::string>("judgedb", "password"),
                cfg.get<std::string>("judgedb", "database", "judgedb")
            };
            if (!pi.isExist(title)) {
                throw Exceptions::ParameterException(VALUE_ERROR, "title", "问题不存在");
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
                    throw Exceptions::ParameterException(MISSING_PARAM, "one of test case fields", "测试用例信息缺失");
                }
            }

            if (!App.uploadTestCases(std::move(cases), title)) {
                throw Exceptions::makeSystemException("upload test cases failed", __FILE__, __LINE__);
            }
            res.status = OK;
            auto response{ njson{{"status", "success"}, {"message", "上传测试通过"}} };
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
