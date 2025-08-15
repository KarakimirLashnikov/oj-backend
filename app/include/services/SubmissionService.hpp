#pragma once

#include "services/ServiceInfo.hpp"
#include "Types.hpp"

namespace OJApp
{
    using Core::Types::SubmissionInfo;
    class SubmissionService
    {
    public:
        SubmissionService() = default;

        ServiceInfo submit(SubmissionInfo info, const std::string &token);
    };
}