#pragma once
#include "Core.hpp"

namespace Judge::Language
{
    enum class LangID : uint8_t
    {
        UNKNOWN = 0,
        CPP,
        PYTHON
    };

    inline std::string getFileExtension(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return ".cpp";
        case LangID::PYTHON:
            return ".py";
        case LangID::UNKNOWN:
        default: return "";
        }
    }
}