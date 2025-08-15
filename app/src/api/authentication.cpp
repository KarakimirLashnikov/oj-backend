#include "api/authentication.hpp"
#include "api/api.hpp"
#include "Application.hpp"

namespace OJApp::Authentication
{
    using namespace Core::Http;
    using Core::Types::UserInfo;

    void registry(const httplib::Request& req, httplib::Response& res)
    {

        handleHttpWrapper(req, res, { "username", "password", "email" }, 
            [&](httplib::Response& res, njson request) -> void{
                UserInfo info{};
                info.username = request.at("username").get<std::string>();
                info.password_hash = request.at("password").get<std::string>();
                info.email = request.at("email").get<std::string>();

                LOG_INFO("check username unique {} and create new user", info.username);
                auto sv_info{ App.getAuthService().registryService(std::move(info)) };

                res.status = sv_info.status;
                res.set_content(sv_info.message.dump(), "application/json");
            }
        );
    }

    void login(const httplib::Request& req, httplib::Response& res)
    {
        handleHttpWrapper(req, res, { "username", "password" }, 
            [&](httplib::Response& res, njson request) -> void{
                UserInfo info {};
                info.username = request.at("username").get<std::string>();
                info.password_hash = request.at("password").get<std::string>();

                LOG_INFO("check user password {} exists", info.username);
                auto sv_info{ App.getAuthService().loginService(std::move(info)) };

                res.status = sv_info.status;
                res.set_content(sv_info.message.dump(), "application/json");
            }
        );
    }
}