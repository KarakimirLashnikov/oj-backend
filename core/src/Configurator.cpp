#include "Configurator.hpp"
#include "FileException.hpp"

namespace Core
{
    Configurator::Configurator(std::string_view conf_file_path)
    : m_ReaderPtr{ nullptr }
    , m_ConfFilePath{ conf_file_path.data() }
    {
        Exceptions::checkFileExists(conf_file_path.data());
        Exceptions::checkFileReadable(conf_file_path.data());

        m_ReaderPtr = std::make_unique<INIReader>(conf_file_path.data());
        if (m_ReaderPtr->ParseError() < 0) {
            throw Exceptions::makeFileException(
                conf_file_path.data(), "Read config and construct Configurator instance failed");
        }
    }
}