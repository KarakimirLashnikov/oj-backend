#include <nlohmann/json.hpp>
#include <sodium.h>
#include "api/Login.hpp"
#include "Types.hpp"
#include "Application.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"
#include "DBManager.hpp"
#include "authentication/AuthService.hpp"

namespace OJApp::Login
{
    using Core::Http::InternalServerError;
    using Core::Http::BadRequest;
    using Core::Http::OK;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;
    using Exceptions::SystemException;
    using Exceptions::ParameterException;

    void login(const httplib::Request &req, httplib::Response &res) 
    {
        try {
            njson j = njson::parse(req.body);
            if (!j.contains("username") ||
                !j.contains("password") ||
                !j.contains("email") ||
                !j.contains("token")) {
                throw ParameterException(
                    Exceptions::ParameterException::ExceptionType::MISSING_PARAM,
                    "username/password/email");
            }
            AuthService& auth_service{ App.getAuthService() };
            std::string error_msg{}, token{ j["token"]};
            if (auth_service.loginUser(j["username"], j["password"], j["email"], token, error_msg)) {
                res.status = OK;
                njson res_content = njson{{"status", "success"}};
                res.set_content(res_content.dump(), "application/json");
                return;
            }

            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", error_msg}, {"token", token}};
            res.set_content(res_content.dump(), "application/json");

        } catch (const Exceptions::ParameterException &e) {
            LOG_INFO("Login Request ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const sql::SQLException &e) {
            LOG_INFO("Database error during login: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"},
                                      {"message", e.what()},
                                      {"code", e.getErrorCode()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const std::exception &e) {
            LOG_INFO("Login failed: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }

    void registry(const httplib::Request &req, httplib::Response &res) 
    {
        try {
            njson j = njson::parse(req.body);
            if (!j.contains("username") ||
                !j.contains("password") ||
                !j.contains("email")) {
                    throw ParameterException(
                    Exceptions::ParameterException::ExceptionType::MISSING_PARAM,
                    "username/password/email");
            }
            AuthService& auth_service{ App.getAuthService() };
            std::string error_msg{};
            if (auth_service.registerUser(j["username"], j["password"], j["email"], error_msg)) {
                res.status = OK;
                njson res_content = njson{{"status", "success"}};
                res.set_content(res_content.dump(), "application/json");
                return;
            }
            
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", error_msg}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const Exceptions::ParameterException &e) {
            LOG_INFO("Signup Request ParameterError: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const sql::SQLException &e) {
            LOG_ERROR("Database error during registry: {}", e.what());
            res.status = InternalServerError;
            njson res_content = njson{{"status", "failed"},
                                      {"message", e.what()},
                                      {"code", e.getErrorCode()}};
            res.set_content(res_content.dump(), "application/json");
        } catch (const std::exception &e) {
            LOG_INFO("Signup error: {}", e.what());
            res.status = BadRequest;
            njson res_content = njson{{"status", "failed"}, {"message", e.what()}};
            res.set_content(res_content.dump(), "application/json");
        }
    }
}

