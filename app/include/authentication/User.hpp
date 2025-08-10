#pragma once
#include "Core.hpp"
#include "Types.hpp"
#include "UserInfo.hpp"

namespace OJApp
{
    using Core::Types::TimeStamp;
    
    class User
    {
    private:
        std::string m_ID;
        std::string m_UserName;
        std::string m_Email;
        std::string m_PasswordHash;
        std::string m_Salt;
        bool m_IsActive;
        TimeStamp m_CreatedAt;
        TimeStamp m_UpdatedAt;

    public:
        User() = default;
        User(Database::UserInfo user_info);

        // Getters
        inline const std::string &getId() const { return m_ID; }
        inline const std::string &getUsername() const { return m_UserName; }
        inline const std::string &getEmail() const { return m_Email; }
        inline const std::string &getPasswordHash() const { return m_PasswordHash; }
        inline const std::string &getSalt() const { return m_Salt; }
        inline bool getIsActive() const { return m_IsActive; }
        inline const TimeStamp &getCreatedAt() const { return m_CreatedAt; }
        inline const TimeStamp &getUpdatedAt() const { return m_UpdatedAt; }

        // Setters
        inline void setId(const std::string &id) { this->m_ID = id; }
        inline void setUsername(const std::string &username) { this->m_UserName = username; }
        inline void setEmail(const std::string &email) { this->m_Email = email; }
        inline void setPasswordHash(const std::string &password_hash) { this->m_PasswordHash = password_hash; }
        inline void setSalt(const std::string &salt) { this->m_Salt = salt; }
        inline void setIsActive(bool is_active) { this->m_IsActive = is_active; }
        inline void setCreatedAt(const TimeStamp &time) { m_CreatedAt = time; }
        inline void setUpdatedAt(const TimeStamp &time) { m_UpdatedAt = time; }
    };
}