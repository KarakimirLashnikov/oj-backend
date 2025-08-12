#include "api/authentication.hpp"
#include "SystemException.hpp"
#include "ParameterException.hpp"
#include "Application.hpp"
#include "Http.hpp"

namespace OJApp::Authentication
{
    using namespace Core::Http;

    void registry(const httplib::Request& req, httplib::Response& res)
    {
        try {
            LOG_INFO("receive request {}", req.body);
            njson request = njson::parse(req.body);
            Exceptions::checkJsonParamExist(request, {"username", "password", "email"});
            UserInfo info{
                .username = request.at("username").get<std::string>(),
                .password_hash = request.at("password").get<std::string>(),
                .email = request.at("email").get<std::string>()
            };
            LOG_INFO("check username unique {} and create new user", info.username);
            if (App.getAuthService().registryService(std::move(info)))
            {
                res.status = Created;
                njson response = njson{ {"message", "registration success"} };
                res.set_content(response.dump(), "application/json");
                return;
            }
            res.status = BadRequest;
            njson response = njson{ {"message", "registration failed"} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = InternalServerError;
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        }
    }

    void login(const httplib::Request& req, httplib::Response& res)
    {}
}