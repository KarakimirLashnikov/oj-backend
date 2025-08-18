#include "services/IAuthRequired.hpp"

using Core::Types::UserInfo;
using OJApp::ServiceInfo;

OJApp::IAuthRequired::IAuthRequired(Core::Configurator &cfg)
: m_TokenManager{ std::make_unique<OJApp::TokenManager>(cfg) }
{
}

std::optional<UserInfo> OJApp::IAuthRequired::authenticate(const std::string &token)
{
    auto user{ m_TokenManager->verifyToken(token) };
    if (!user) {
        return std::nullopt;
    }
    return user;
}

ServiceInfo OJApp::IAuthRequired::createAuthFailedResp() const
{
    ServiceInfo info;
    info.message["message"] = "Invalid or expired token!";
    info.status = ResponseStatusCode::Unauthorized;
    return info;
}