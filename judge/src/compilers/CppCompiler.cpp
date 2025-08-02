#include "compilers/CppCompiler.hpp"
namespace bp = boost::process;

namespace Judge
{
    bool CppCompiler::compile(std::string_view source_path,
                              std::string_view output_path,
                              const std::initializer_list<const char *>& additional_flags)
    {
        // 清空之前的输出
        this->m_CompileMessage.clear();

        try {
            // 安全地查找编译器路径（避免命令注入）
            std::string compiler{ this->m_GppPath };
            if (compiler.empty()) {
                compiler = bp::search_path("g++").string();
                if (compiler.empty()) {
                    this->m_CompileMessage = "Error: g++ compiler not found in PATH";
                    return false;
                }
            }

            // 构建安全参数列表（避免shell注入）
            std::vector<std::string> args;
            args.push_back(compiler);

            // 添加基础编译选项
            args.insert(args.end(), {"-Wall", "-Werror", "-Wextra",
                                     "-o", std::string(output_path),
                                     std::string(source_path)});
            // 处理额外标志（安全分割）
            for (const char* flag : additional_flags) {
                args.push_back(flag);
            }

            // 准备捕获输出
            bp::ipstream out_stream;
            bp::ipstream err_stream;

            // 启动编译进程（不使用shell）
            bp::child c(
                args,
                bp::std_out > out_stream,
                bp::std_err > err_stream);

            // 并行读取标准输出和错误
            std::string line;
            while (c.running() &&
                   (std::getline(out_stream, line) ||
                    std::getline(err_stream, line))) {
                this->m_CompileMessage += line + "\n";
            }

            // 等待进程结束
            c.wait();

            // 捕获剩余输出
            while (std::getline(out_stream, line))
                this->m_CompileMessage += line + "\n";
            while (std::getline(err_stream, line))
                this->m_CompileMessage += line + "\n";

            return c.exit_code() == 0;
        }
        catch (const std::exception &e) {
            this->m_CompileMessage = std::format("Compilation failed: {}", e.what());
            return false;
        }
    }
}
