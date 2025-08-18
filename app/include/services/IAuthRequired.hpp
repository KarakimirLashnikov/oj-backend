#pragma once

#include "services/TokenManager.hpp"
#include "services/ServiceInfo.hpp"

namespace OJApp
{
    class IAuthRequired
    {
    public:
        IAuthRequired(Core::Configurator &cfg);

        ~IAuthRequired() = default;

    protected:
        std::optional<UserInfo> authenticate(const std::string &token);

        ServiceInfo createAuthFailedResp() const;

    protected:
        std::unique_ptr<TokenManager> m_TokenManager;
    };
}