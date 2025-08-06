#pragma once
#include <cpp/INIReader.h>
#include "Core.hpp"
#include "Types.hpp"

namespace Core
{
    class Configurator
    {
    public:
        Configurator(std::string_view conf_file_path = "config.ini");

        template <Types::IniSupportedType TValue>
        TValue get(std::string_view section, std::string_view key, TValue default_value = {}) const
        {
            if constexpr (std::is_integral_v<TValue>) {
                return static_cast<TValue>(m_ReaderPtr->GetInteger(section.data(), key.data(), default_value));
            }
            else if constexpr (std::is_same_v<TValue, std::string>) {
                return m_ReaderPtr->GetString(section.data(), key.data(), default_value.data());
            }
            else if constexpr (std::is_floating_point_v<TValue>) {
                return static_cast<TValue>(m_ReaderPtr->GetReal(section.data(), key.data(), default_value));
            }
            else if constexpr (std::is_same_v<TValue, bool>) {
                return static_cast<TValue>(m_ReaderPtr->GetBoolean(section.data(), key.data(), default_value));
            }
            else {
                return default_value;
            }
        }

        inline const fs::path &getConfFilePath() const noexcept { return m_ConfFilePath; }

    private:
        std::unique_ptr<INIReader> m_ReaderPtr;
        fs::path m_ConfFilePath;
    };
}