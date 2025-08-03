#pragma once
#include "Core.hpp"

namespace Judge
{
    class Compiler
    {
    public:
        [[nodiscard]] virtual bool compile(std::string_view source_path, 
                                        std::string_view output_path, 
                                        const std::vector<std::string>& additional_flags) = 0;
        [[nodiscard]] virtual std::string getCompileMessage() const = 0;
        virtual ~Compiler() = default;
    };

    template <typename TCompiler, typename... Args>
    requires std::derived_from<TCompiler, Compiler>
    inline std::unique_ptr<TCompiler> createCompiler(Args&&... args) { 
        return std::make_unique<TCompiler>(std::forward<Args>(args)...);
    }
}