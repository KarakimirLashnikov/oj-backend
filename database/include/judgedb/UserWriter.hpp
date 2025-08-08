#pragma once
#include "MySQLDB.hpp"
#include "Types.hpp"

namespace JudgeDB
{
    using Core::Types::TimeStamp;
    class UserWriter : public MySQLDB::Database
    {
    public:
        UserWriter(std::string_view host
                  , uint16_t port
                  , std::string_view user
                  , std::string_view password
                  , std::string_view database);

        virtual ~UserWriter() = default;

        bool createUser(std::string_view username // 业务数据
                       , std::string_view email 
                       , std::string_view password);
    };
}