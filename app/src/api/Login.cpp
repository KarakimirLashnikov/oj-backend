#include <nlohmann/json.hpp>
#include <sodium.h>
#include "api/Login.hpp"
#include "Types.hpp"
#include "Application.hpp"
#include "judgedb/UserInquirer.hpp"
#include "judgedb/UserWriter.hpp"
#include "ParameterException.hpp"
#include "SystemException.hpp"

namespace OJApp::Login
{
    using Core::Http::InternalServerError;
    using Core::Http::BadRequest;
    using Core::Http::OK;
    using Exceptions::ParameterException::ExceptionType::MISSING_PARAM;
    using Exceptions::ParameterException::ExceptionType::VALUE_ERROR;

    void login(const httplib::Request &req, httplib::Response &res) {
        try {
            res.status = OK;
            njson response = njson{{"status", "success"}, {"message", "login succeed"}};
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
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }

    void signup(const httplib::Request &req, httplib::Response &res) {
        try {
            res.status = OK;
            njson response = njson{{"status", "success"}, {"message", "signup succeed"}};
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
            res.status = InternalServerError;
            auto json{ njson{{"status", "failure"}, {"message", e.what()}} };
            res.set_content(json.dump(), "application/json");
        }
    }
}

