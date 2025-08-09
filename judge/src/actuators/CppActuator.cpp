#include "actuators/CppActuator.hpp"

namespace Judge
{
    void CppActuator::runChildProcess(const fs::path& exe_path)
    {
        char* args[] = {const_cast<char*>(exe_path.filename().c_str()), nullptr};
        execv(exe_path.c_str(), args);
    }
}