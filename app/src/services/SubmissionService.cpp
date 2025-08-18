#include "services/SubmissionService.hpp"
#include "Application.hpp"
#include "dbop/DbOpFactory.hpp"

namespace OJApp
{

    SubmissionService::SubmissionService(Core::Configurator& cfg)
    : IAuthRequired{ cfg }
    , m_SubPrefix{ cfg.get<std::string>("application", "SUBMISSION_PREFIX", "submission:")}
    {}


    using DbOp::makeDbOp;
    using DbOp::OpType;
    ServiceInfo SubmissionService::submit(SubmissionInfo info, const std::string &token)
    {
        ServiceInfo sv_info{};
        
        if (!this->authenticate(token)) {
            sv_info = this->createAuthFailedResp();
            return sv_info;
        }

        info.submission_id = boost::uuids::to_string(boost::uuids::random_generator()());
        sv_info.message["submission_id"] = info.submission_id;

        std::string data_key{ m_SubPrefix + info.submission_id };

        App.getRedisManager().set(data_key, info.toJson().dump()) ;
        App.processJudgeTask(std::move(info));

        sv_info.status = Created;
        sv_info.message["message"] = "Submission successful, you can check the status on the status panel";
        return sv_info;
    }

    ServiceInfo SubmissionService::querySubmission(const std::string &submission_id, const std::string &token)
    {
        ServiceInfo sv_info{};

        if (!this->authenticate(token)) {
            sv_info = this->createAuthFailedResp();
            return sv_info;
        }

        if (App.getRedisManager().exists(m_SubPrefix + submission_id)) {
            sv_info.status = OK;
            sv_info.message["message"] = "query succeed";
            sv_info.message["submission_status"] = App.getRedisManager().get(m_SubPrefix + submission_id);
            return sv_info;
        }

        auto db_op = makeDbOp(OpType::Query
                            , R"SQL(SELECT overall_status FROM submissions WHERE submission_id = ? LIMIT 1;)SQL"
                            , { submission_id });
        auto result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get()));
        if (result.empty()) {
            LOG_WARN("submission_id[{}] query failed", submission_id);
            sv_info.message["message"] = "query failed";
            sv_info.status = NotFound;
            return sv_info;
        }

        sv_info.status = OK;
        sv_info.message["message"] = "query succeed";
        sv_info.message["submission_status"] = result[0]["overall_status"];
        return sv_info;
    }
}
