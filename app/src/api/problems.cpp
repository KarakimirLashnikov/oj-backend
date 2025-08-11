#include <nlohmann/json.hpp>
#include "api/problems.hpp"
#include "Types.hpp"
#include "Http.hpp"
#include "Application.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"
#include "AuthService.hpp"
#include "api/problems.hpp"

namespace OJApp::Problems
{
    using Core::Types::DifficultyLevel;
    using Core::Http::OK;
    using Core::Http::BadRequest;
    using Core::Http::InternalServerError;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;
    using Exceptions::SystemException;
    using Exceptions::ParameterException;

    void createProblem(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson j = njson::parse(req.body);
            Exceptions::checkJsonParamExist(j, {"token", "title", "description", "difficulty"});
            AuthService& auth_service{ App.getAuthService() };
            auto user_info{ auth_service.getCurrentUser(j.at("token").get<std::string>()) };
            if (!user_info)
                throw ParameterException(VALUE_ERROR, "token");

            Database::DBManager& db{ App.getDBManager() };
            Database::ProblemInfo info{
                .author_uuid = user_info.value().user_uuid,
                .title = j.at("title").get<std::string>(),
                .description = j.at("description").get<std::string>(),
                .level = Core::Types::stringToDifficulty(j.at("difficulty").get<std::string>()),
            };
            App.addDBTask(db.insertProblem(info));
            res.status = OK;
            njson res_content = njson{{"status", "success"}, {"message", "create succeed"}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::ParameterException &e) {
            LOG_INFO("CreateRequest ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const sql::SQLException &e) {
            LOG_INFO("Database error during create: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::SystemException &e) {
            LOG_INFO("System error during create: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }

    using Judge::Language::LangID;
    void addProblemLimit(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson j = njson::parse(req.body);
            Exceptions::checkJsonParamExist(j, {"token", "problem_title", "language_id", "time_limit_s", "memory_limit_kb"});
            AuthService& auth_service{ App.getAuthService() };
            auto user_info{ auth_service.getCurrentUser(j.at("token").get<std::string>()) };
            if (!user_info)
                throw ParameterException(VALUE_ERROR, "token");
                
            Database::DBManager& db{ App.getDBManager() };
            Judge::ResourceLimits limits{
                .time_limit_s = j.at("time_limit_s").get<float>(),
                .extra_time_s = j.contains("extra_time_s") ? j.at("extra_time_s").get<float>() : 0.5f,
                .wall_time_s = j.contains("wall_time_s") ? j.at("wall_time_s").get<float>() : j.at("time_limit_s").get<float>() + 1.5f,
                .memory_limit_kb = j.at("memory_limit_kb").get<uint32_t>(),
                .stack_limit_kb = j.contains("stack_limit_kb") ? j.at("stack_limit_kb").get<uint32_t>() : j.at("memory_limit_kb").get<uint32_t>()
            };
            LangID language_id = static_cast<LangID>(j.at("language_id").get<int>());
            App.addDBTask(db.insertProblemLimits(j.at("problem_title").get<std::string>(), language_id, limits));
            res.status = OK;
            njson res_content = njson{{"status", "success"}, {"message", "add limits succeed"}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::ParameterException &e) {
            LOG_INFO("AddJudgeLimit Request ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const sql::SQLException &e) {
            LOG_INFO("Database error during add judge limit: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::SystemException &e) {
            LOG_INFO("System error during add judge limit: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }

    using Core::Types::TestCase;
    void uploadTestCases(const httplib::Request &req, httplib::Response &res)
    {
        try {
            njson j = njson::parse(req.body);
            Exceptions::checkJsonParamExist(j, {"token", "problem_title", "testcase_list"});
            if (!j.at("testcase_list").is_array())
                throw ParameterException(VALUE_ERROR, "testcase_list");
            AuthService& auth_service{ App.getAuthService() };
            auto user_info{ auth_service.getCurrentUser(j.at("token").get<std::string>()) };
            if (!user_info)
                throw ParameterException(VALUE_ERROR, "token");
            std::vector<TestCase> tcs_list{};
            for (const auto &t : j.at("testcase_list")) {
                Exceptions::checkJsonParamExist(t, {"stdin_data", "expected_output", "sequence"});
                TestCase tc{
                    .stdin_data = t.at("stdin_data").get<std::string>(),
                    .expected_output = t.at("expected_output").get<std::string>(),
                    .sequence = t.at("sequence").get<uint32_t>()};
                if (t.at("sequence") < 1)
                    throw ParameterException(VALUE_ERROR, "sequence", "must be an integer, greater than zero");
                tcs_list.push_back(std::move(tc));
            }

            Database::DBManager& db{ App.getDBManager() };
            App.addDBTask(db.insertTestCases(std::move(tcs_list), j.at("problem_title").get<std::string>()));

            res.status = OK;
            njson res_content = njson{{"status", "success"}, {"message", "upload testcase succeed"}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::ParameterException &e) {
            LOG_INFO("CreateRequest ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const sql::SQLException &e) {
            LOG_INFO("Database error during create: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::SystemException &e) {
            LOG_INFO("System error during create: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }
}
