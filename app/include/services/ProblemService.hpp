#pragma once
#include "services/ServiceInfo.hpp"
#include "services/IAuthRequired.hpp"

namespace OJApp
{
    using Core::Types::ProblemInfo;
    using Core::Types::ProblemLimitsInfo;
    using Core::Types::TestCaseInfo;

    class ProblemService : public IAuthRequired
    {
    public:
        ProblemService(Core::Configurator& cfg);

        ServiceInfo createProblem(ProblemInfo info, const std::string& token);

        ServiceInfo addProblemLimit(ProblemLimitsInfo info, const std::string& token);

        ServiceInfo uploadTestCases(TestCaseInfo info, const std::string& token);

        ServiceInfo getProblemList(const std::string& token);

        inline std::string getProblemPrefix() const { return m_ProblemPrefix; }
        inline std::string getLimitPrefix() const { return m_LimitPrefix; }
        inline std::string getTestCasePrefix() const { return m_TestCasePrefix; }
    private:
        std::string m_ProblemPrefix;
        std::string m_LimitPrefix;
        std::string m_TestCasePrefix;
    };
}