#include "FileWriter.hpp"
#include "FileException.hpp"
#include "Logger.hpp"

namespace Core
{
    FileWriter::FileWriter(asio::io_context &ioc)
        : m_IO{ioc}
    {
    }

    void FileWriter::write(std::string filename, std::string content)
    {
        int fd = open(filename.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        auto sd = std::make_shared<asio::posix::stream_descriptor>(m_IO, fd);
        auto buf = std::make_shared<std::vector<char>>(content.begin(), content.end());
        asio::async_write(*sd, asio::buffer(*buf), [sd, buf](
            const boost::system::error_code &ec, std::size_t) {
                sd->close();
            }
        );
    }
}