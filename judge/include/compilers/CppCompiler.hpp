#pragma once
#include "compilers/Compiler.hpp"

namespace Judge
{
    class CppCompiler : public Compiler
    {
    public:
        CppCompiler() = default;

        bool compile(std::string_view source_path,
                     std::string_view output_path,
                     const std::initializer_list<const char *>& additional_flags = {"-O2", "std=c++11"}) override;

        inline std::string getCompileMessage() const override {
            return this->m_CompileMessage;
        }

        virtual ~CppCompiler() = default;

    private:
        std::string m_GppPath;
        std::string m_CompileMessage;
    };
}