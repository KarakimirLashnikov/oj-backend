#pragma once

#include "Core.hpp"

namespace Database
{
    struct UserInfo
    {
        std::string username;
        std::string password_hash;
        std::string email;
        std::string salt;
    };
}