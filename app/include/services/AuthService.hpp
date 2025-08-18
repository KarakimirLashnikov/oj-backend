#pragma once
#include "Types.hpp"
#include "Configurator.hpp"
#include "services/ServiceInfo.hpp"
#include "services/IAuthRequired.hpp"

namespace OJApp
{
    using Core::Types::UserInfo;

    class AuthService : public IAuthRequired
    {
    public:
        AuthService(Core::Configurator& cfg);

        ServiceInfo registryService(UserInfo info);

        ServiceInfo loginService(UserInfo info);

    private:
        bool queryUserNameExist(std::string_view name);

        std::optional<std::string> hashPassword(std::string_view pwd);

        bool verifyPassword(std::string_view password, std::string_view hash);
    };
}