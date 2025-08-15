#pragma once
#include "services/ServiceInfo.hpp"
#include "Types.hpp"

namespace OJApp
{
    using Core::Types::ProblemInfo;
    using Core::Types::ProblemLimitsInfo;
    using Core::Types::TestCaseInfo;

    class ProblemService
    {
    public:
        ProblemService() = default;

        ServiceInfo createProblem(ProblemInfo info, const std::string& token);

        ServiceInfo addProblemLimit(ProblemLimitsInfo info, const std::string& token);

        ServiceInfo uploadTestCases(TestCaseInfo info, const std::string& token);
    };
}