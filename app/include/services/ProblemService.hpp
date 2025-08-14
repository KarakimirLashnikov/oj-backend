#pragma once
#include "Configurator.hpp"
#include "services/ServiceInfo.hpp"
#include "Types.hpp"

namespace OJApp
{
    using Core::Types::ProblemInfo;
    using Core::Types::LimitsInfo;

    class ProblemService
    {
    public:
        ProblemService() = default;

        ServiceInfo createProblem(ProblemInfo info, const std::string& token);
    };
}