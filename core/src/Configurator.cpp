#include "Configurator.hpp"
#include "Logger.hpp"

namespace Core
{
    Configurator::Configurator(std::string_view conf_file_path)
    : m_ReaderPtr{ nullptr }
    {
        if (conf_file_path.empty() || !std::filesystem::exists(conf_file_path)) {
            LOG_ERROR("Configuration file {} doesn't exist!", conf_file_path.data());
            throw std::runtime_error{"Configuration file doesn't exist"};
        }

        m_ReaderPtr = std::make_unique<INIReader>(conf_file_path.data());
        if (m_ReaderPtr->ParseError() < 0) {
            LOG_ERROR("Parse Configuration file {} error!", conf_file_path.data());
            throw std::runtime_error{"Parse Configuration file error"};
        }
    }
}