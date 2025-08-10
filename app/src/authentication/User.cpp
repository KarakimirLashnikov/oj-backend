#include "authentication/User.hpp"

OJApp::User::User(Database::UserInfo info)
    : m_UserName{info.username}
    , m_Email{info.email}
    , m_PasswordHash{info.password_hash}
    , m_Salt{info.salt}
    , m_IsActive{true}
    , m_CreatedAt{std::chrono::steady_clock::now()}
    , m_UpdatedAt{std::chrono::steady_clock::now()}
{
}