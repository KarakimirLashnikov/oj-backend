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

            UserInfo info{};
            info.username = request.at("username").get<std::string>();
            info.password_hash = request.at("password").get<std::string>();
            info.email = request.at("email").get<std::string>();

            LOG_INFO("check username unique {} and create new user", info.username);
            auto sv_info{ App.getAuthService().registryService(std::move(info)) };

            res.status = sv_info.status;
            njson response = njson{ {"message", sv_info.message} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::SystemException& e) {
            res.status = e.errorCode();
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        }
    }

    void login(const httplib::Request& req, httplib::Response& res)
    {
        try {
            LOG_INFO("receive request {}", req.body);

            njson request = njson::parse(req.body);
            Exceptions::checkJsonParamExist(request, {"username", "password"});

            UserInfo info {};
            info.username = request.at("username").get<std::string>();
            info.password_hash = request.at("password").get<std::string>();
            std::string token{};

            LOG_INFO("check user password {} exists", info.username);
            auto sv_info{ App.getAuthService().loginService(std::move(info), token) };

            res.status = sv_info.status;
            njson response = njson{ {"message", sv_info.message}, {"token", token} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::ParameterException& e) {
            res.status = BadRequest;
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        } catch (const Exceptions::SystemException& e) {
            res.status = e.errorCode();
            njson response = njson{ {"message", e.what()} };
            res.set_content(response.dump(), "application/json");
        }
    }
}