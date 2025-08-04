#pragma once
#include "Core.hpp"
#include "Types.hpp"

namespace Core
{

    class FileWriter {
    public:
        FileWriter(asio::io_context& io);
        
        void write(std::string_view filename, const std::string& content);

    private:
        asio::io_context& m_IO;
    };
}