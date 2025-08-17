#include "actuators/PythonActuator.hpp"
#include "SystemException.hpp"
#include "FileException.hpp"
#include "Logger.hpp"

static std::once_flag ONCE_FLAG{};
static std::vector<std::string> PYTHON_EXTRA_ARGS{};
static fs::path PYTHON_INTERPRETER{};
static std::string PYTHON_SAFEPATH{};
static std::string PYTHON_DONTWRITEBYTECODE{};
static std::string PYTHON_IOENCODING{};

namespace Judge
{
    void PythonActuator::setupLanguageEnv()
    {
        // Python安全环境变量
        std::call_once(::ONCE_FLAG, []() {
            ::PYTHON_INTERPRETER = Actuator::s_ConfiguratorPtr->get<std::string>("python3", "PYTHON_INTERPRETER", "/usr/bin/python3");
            ::PYTHON_SAFEPATH = Actuator::s_ConfiguratorPtr->get<std::string>("python3", "PYTHON_SAFEPATH", "");
            ::PYTHON_DONTWRITEBYTECODE = Actuator::s_ConfiguratorPtr->get<std::string>("python3", "PYTHON_DONTWRITEBYTECODE", "-B");
            ::PYTHON_IOENCODING = Actuator::s_ConfiguratorPtr->get<std::string>("python3", "PYTHON_IOENCODING", "");
            std::string extra_args_tmp{ Actuator::s_ConfiguratorPtr->get<std::string>("python3", "PYTHON_EXTRA_ARGS", "") };
            boost::split(::PYTHON_EXTRA_ARGS, extra_args_tmp, boost::is_any_of(","));
        });

        
        if (!::PYTHON_SAFEPATH.empty()) {
            setenv("PYTHONSAFEPATH", ::PYTHON_SAFEPATH.c_str(), 1);
        }
        if (!::PYTHON_DONTWRITEBYTECODE.empty()) {
            setenv("PYTHONDONTWRITEBYTECODE", ::PYTHON_DONTWRITEBYTECODE.c_str(), 1);
        }
        if (!::PYTHON_IOENCODING.empty()) {
            setenv("PYTHONIOENCODING", ::PYTHON_IOENCODING.c_str(), 1);
        }
    }

    void PythonActuator::runChildProcess(const fs::path &exe_path) 
    {
        std::vector<char*> argv;
        
        // 1. 添加 Python 解释器路径
        argv.push_back(const_cast<char*>(::PYTHON_INTERPRETER.c_str()));
        
        // 2. 添加其他附加参数
        for (const auto& arg : ::PYTHON_EXTRA_ARGS) {
            if (!arg.empty()) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
        }
        
        // 3. 添加要执行的可执行文件路径
        argv.push_back(const_cast<char*>(exe_path.c_str()));
        
        // 4. 添加 NULL 分隔符
        argv.push_back(nullptr);
        
        // 5. 执行
        execv(::PYTHON_INTERPRETER.c_str(), argv.data());
    }
}