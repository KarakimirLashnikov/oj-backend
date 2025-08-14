#pragma once
#include "Types.hpp"
#include "Configurator.hpp"
#include "services/ServiceInfo.hpp"

namespace OJApp
{
    using Core::Types::UserInfo;

    class AuthService
    {
    public:
        AuthService(Core::Configurator& cfg);

        ServiceInfo registryService(UserInfo info);

        ServiceInfo loginService(UserInfo info, std::string& token);

        bool queryUserNameExist(std::string_view name);

    private:
        std::optional<std::string> generateUserToken(std::string_view username);

        std::optional<std::string> hashPassword(std::string_view pwd);

        bool verifyPassword(std::string_view password, std::string_view hash);

    private:
        std::string m_SecretKey;
        std::chrono::seconds m_ExpireSec;
    };
}