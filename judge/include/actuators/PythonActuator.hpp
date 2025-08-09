#pragma once
#include "actuators/Actuator.hpp"

namespace Judge
{
    class PythonActuator : public Actuator
    {
    public:
        PythonActuator() = default;
        virtual ~PythonActuator() = default;

    private:
        void setupLanguageEnv() override;
        void runChildProcess(const fs::path& exe_path) override;
    };
}