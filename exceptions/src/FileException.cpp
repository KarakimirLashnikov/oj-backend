#include "FileException.hpp"

namespace Exceptions
{
    FileException::FileException(ErrorType type, const std::string &path, const std::string &message)
        : std::runtime_error(message), m_type(type), m_path(path), m_errorCode(0)
    {
    }

    FileException::FileException(ErrorType type, const std::string &path, const std::string &message, int errorCode)
        : std::runtime_error(message), m_type(type), m_path(path), m_errorCode(errorCode)
    {
    }

    const char *FileException::errorTypeToString(ErrorType type)
    {
        switch (type)
        {
        case ErrorType::FileNotFound:
            return "File not found";
        case ErrorType::PermissionDenied:
            return "Permission denied";
        case ErrorType::OpenFailed:
            return "Open failed";
        case ErrorType::ReadError:
            return "Read error";
        case ErrorType::WriteError:
            return "Write error";
        case ErrorType::SeekError:
            return "Seek error";
        case ErrorType::AlreadyExists:
            return "File already exists";
        case ErrorType::InvalidOperation:
            return "Invalid operation";
        case ErrorType::UnknownError:
            return "Unknown error";
        default:
            return "Unspecified error";
        }
    }

    const char *FileException::what() const noexcept
    {
        if (m_whatCache.empty())
        {
            m_whatCache = std::string(std::runtime_error::what()) +
                          "\nError type: " + errorTypeToString(m_type) +
                          "\nPath: " + m_path;
            if (m_errorCode != 0)
            {
                m_whatCache += "\nSystem error code: " + std::to_string(m_errorCode);
            }
        }
        return m_whatCache.c_str();
    }

    void checkFileExists(const std::string &path)
    {
        if (!fs::exists(path)) {
            throw FileException(FileException::ErrorType::FileNotFound, path, "File or directory does not exist");
        }
    }

    // 辅助函数：根据系统错误创建适当的文件异常
    FileException makeFileException(const std::string &path, const std::string &operation)
    {
        int err = errno; // 获取系统错误码

        // 根据错误码确定异常类型
        FileException::ErrorType type;
        std::string message = operation.empty() ? "File operation failed" : "Failed to " + operation + " file";

        switch (err)
        {
        case ENOENT: // 文件不存在
            type = FileException::ErrorType::FileNotFound;
            message = "File not found";
            break;
        case EACCES: // 权限不足
            type = FileException::ErrorType::PermissionDenied;
            message = "Permission denied";
            break;
        case EEXIST: // 文件已存在
            type = FileException::ErrorType::AlreadyExists;
            message = "File already exists";
            break;
        case EIO: // 输入输出错误
            type = (operation == "read")    ? FileException::ErrorType::ReadError
                   : (operation == "write") ? FileException::ErrorType::WriteError
                                            : FileException::ErrorType::UnknownError;
            message = "I/O error occurred";
            break;
        default:
            type = FileException::ErrorType::UnknownError;
            message = "Unknown file operation error";
            break;
        }

        return FileException(type, path, message, err);
    }

    static constexpr std::array<const char*, 2> SPECIAL_PATHS = {"/proc", "/sys"};

    static bool isSpecialPath(const std::string& path) {
        for (const auto& sp : SPECIAL_PATHS) {
            if (path.find(sp) == 0) return true;
        }
        return false;
    }

    void checkFileReadable(const std::string &path)
    {
        checkFileExists(path);

        // root用户无需检查权限
        if (geteuid() == 0 && isSpecialPath(path)) {
            return; // 跳过特殊路径检查
        }

        std::error_code ec;
        fs::file_status status = fs::status(path, ec);

        if (ec)
        {
            throw FileException(FileException::ErrorType::UnknownError, path,
                                "Failed to check file status", ec.value());
        }

        if ((status.permissions() & fs::perms::owner_read) == fs::perms::none)
        {
            throw FileException(FileException::ErrorType::PermissionDenied, path,
                                "No read permission for file");
        }
    }

    void checkFileWritable(const std::string &path)
    {
        std::error_code ec;
        fs::file_status status = fs::status(path, ec);

        // 文件不存在时跳过权限检查（允许创建）
        if (ec && ec.value() != ENOENT) {
            throw FileException(FileException::ErrorType::FileNotFound, path,
                                "Failed to check file status", ec.value());
        }

        if (geteuid() == 0 && isSpecialPath(path)) {
            return; // 跳过特殊路径检查
        }

        // 如果路径存在（文件或目录），检查写权限
        if (fs::exists(path)) {
            if ((status.permissions() & fs::perms::owner_write) == fs::perms::none) {
                std::string type = fs::is_directory(path) ? "directory" : "file";
                throw FileException(FileException::ErrorType::PermissionDenied, path,
                                    "No write permission for " + type);
            }
        }
    }

    void checkFileExecutable(const std::string &path)
    {
        try {
            checkFileExists(path);

            // root用户可以执行所有文件
            if (geteuid() == 0 && isSpecialPath(path)) {
                return; // 跳过特殊路径检查
            }

            // 如果是目录，检查执行权限（可进入目录）
            if (fs::is_directory(path)) {
                auto perms = fs::status(path).permissions();
                if ((perms & fs::perms::owner_exec) == fs::perms::none) {
                    throw FileException(FileException::ErrorType::PermissionDenied,
                                       path, "Directory is not executable (cannot be accessed)");
                }
                return; // 目录通过检查
            }

            // 文件：检查执行权限
            auto perms = fs::status(path).permissions();
            if ((perms & fs::perms::owner_exec) != fs::perms::none) {
                return; // 有执行权限
            }

            // 无执行权限时检查文件格式
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                throw makeFileException(path, "open for execution check");
            }

            std::array<char, 4> header{};
            file.read(header.data(), header.size());

            // 检查可执行文件签名
            static constexpr std::array<char, 4> elf_magic = {0x7F, 'E', 'L', 'F'};
            static constexpr std::array<char, 2> pe_magic = {'M', 'Z'};
            static constexpr std::array<char, 2> shebang = {'#', '!'};

            if (std::equal(elf_magic.begin(), elf_magic.end(), header.begin()) ||
                std::equal(pe_magic.begin(), pe_magic.end(), header.begin()) ||
                std::equal(shebang.begin(), shebang.end(), header.begin())) 
            {
                return; // 有效的可执行格式
            }

            throw FileException(FileException::ErrorType::PermissionDenied,
                                path, "File is not executable and lacks executable format signature");
        } catch (const fs::filesystem_error &e) {
            throw FileException(FileException::ErrorType::UnknownError,
                                path, e.what(), e.code().value());
        }
    }

    void checkFileAccessible(const std::string &path, int mode)
    {
        checkFileExists(path);
        // root 用户无需权限检查
        if (geteuid() == 0 && isSpecialPath(path)) {
            return; // 跳过特殊路径检查
        }
        
        if (access(path.c_str(), mode) != 0) {
            std::string operation;
            if (mode & R_OK) operation += "read";
            if (mode & W_OK) operation += (operation.empty() ? "" : "/") + std::string("write");
            if (mode & X_OK) operation += (operation.empty() ? "" : "/") + std::string("execute");
            
            throw makeFileException(path, operation + " access");
        }
    }
}