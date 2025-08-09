#pragma once
#include "Core.hpp"
#include "actuators/CppActuator.hpp"
#include "compilers/CppCompiler.hpp"

namespace Judge::Language
{
    enum class LangID : uint8_t
    {
        UNKNOWN = 0,
        CPP,
        PYTHON
    };

    std::string toString(LangID id);

    std::string getFileExtension(LangID id);
}