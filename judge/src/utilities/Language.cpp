#include "utilities/Language.hpp"

namespace Judge::Language
{
    std::string getLanguage(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return "C++";
        case LangID::PYTHON:
            return "Python";
        case LangID::UNKNOWN:
        default: return "unknown";
        }
    }

    std::string getFileExtension(LangID id)
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

    std::unique_ptr<Actuator> getActuator(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return createActuator<CppActuator>();
        case LangID::PYTHON:
            // TODO: return createActuator<PythonActuator>();
        case LangID::UNKNOWN:
        default: return nullptr;
        }
    }

    std::unique_ptr<Compiler> getCompiler(LangID id)
    {
        switch (id)
        {
        case LangID::CPP:
            return createCompiler<CppCompiler>();
        case LangID::PYTHON:
        case LangID::UNKNOWN:
        default: return nullptr;
        }
    }
}