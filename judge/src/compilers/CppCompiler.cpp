#include "compilers/CppCompiler.hpp"
#include "Types.hpp"
#include "FileException.hpp"

namespace Judge
{
    bool CppCompiler::compile(std::string_view source_path,
                              std::string_view output_path,
                              const std::vector<std::string> &additional_flags)
    {
        Exceptions::checkFileExists(source_path.data());
        Exceptions::checkFileReadable(source_path.data());
        // 清空之前的输出
        this->m_CompileMessage.clear();

        // 查找编译器路径
        std::string compiler{this->m_GppPath};
        if (compiler.empty())
        {
            compiler = bp::search_path("g++").string();
            if (compiler.empty())
            {
                throw Exceptions::makeFileException(
                    "Failed to find a valid g++ compiler path", "CppCompiler::compile");
            }
        }

        // 构建参数列表
        std::vector<std::string> args;
        args.push_back(compiler);

        // 添加基础编译选项
        args.insert(args.end(), {"-Wall", "-Werror", "-Wextra",
                                 "-o", std::string(output_path),
                                 std::string(source_path)});
        // 处理额外标志
        for (const std::string &flag : additional_flags)
            args.push_back(flag);
        

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
                std::getline(err_stream, line)))
        {
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
}
