#pragma once
#include "Types.hpp"
#include "Configurator.hpp"
#include "Http.hpp"

namespace OJApp
{
    using Core::Types::UserInfo;
    using namespace Core::Http;

    struct ServiceInfo {
        std::string message;
        ResponseStatusCode status;

        inline operator bool() const { return isSuccess(this->status); }
    };

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

        bool redisExist(std::string_view key) const;

    private:
        std::string m_SecretKey;
        std::chrono::seconds m_ExpireSec;
    };
}