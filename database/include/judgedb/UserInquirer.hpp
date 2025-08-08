#pragma once
#include "MySQLDB.hpp"

namespace JudgeDB
{
    class UserInquirer : public MySQLDB::Database
    {
    public:
        UserInquirer(std::string_view host
                    , std::uint16_t port
                    , std::string_view user
                    , std::string_view password
                    , std::string_view database);

        virtual ~UserInquirer() = default;

        bool isExist(std::string_view username) const;

        std::string getPassword(std::string_view username) const;
    };
}