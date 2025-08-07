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
                  , std::string_view username // 数据库登陆用户名
                  , std::string_view password
                  , std::string_view database);

        virtual ~UserWriter() = default;

        bool createUser(std::string_view username // 业务数据
                       , std::string_view email 
                       , std::string_view password);
    };
}