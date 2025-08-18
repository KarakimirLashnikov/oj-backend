#pragma once

#include "services/ServiceInfo.hpp"
#include "services/IAuthRequired.hpp"

namespace OJApp
{
    using Core::Types::SubmissionInfo;
    class SubmissionService : public IAuthRequired
    {
    public:
        SubmissionService(Core::Configurator& cfg);

        ServiceInfo submit(SubmissionInfo info, const std::string &token);

        ServiceInfo querySubmission(const std::string &submission_id, const std::string &token);

    private:
        std::string m_SubPrefix;
    };
}