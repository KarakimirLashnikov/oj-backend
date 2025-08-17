#include "services/SubmissionService.hpp"
#include "Application.hpp"
#include "dbop/DbOpFactory.hpp"

namespace OJApp
{
    using DbOp::makeDbOp;
    using DbOp::OpType;
    ServiceInfo SubmissionService::submit(SubmissionInfo info, const std::string &token)
    {
        ServiceInfo sv_info{};
        
        if (!App.getRedisManager().exists(token)) {
            sv_info.message["message"] = "Authentication failed, please login again";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        if (!App.getRedisManager().expire(token)) {
            sv_info.message["message"] = "Authentication  extend failed in [submit]";
            sv_info.status = InternalServerError;
            return sv_info;
        }

        info.submission_id = boost::uuids::to_string(boost::uuids::random_generator()());
        sv_info.message["submission_id"] = info.submission_id;

        App.getRedisManager().set("submission_tmp_" + info.submission_id, "PENDING") ;
        App.processJudgeTask(std::move(info));

        sv_info.status = Created;
        sv_info.message["message"] = "Submission successful, you can check the status on the status panel";
        return sv_info;
    }

    ServiceInfo SubmissionService::querySubmission(const std::string &submission_id, const std::string &token)
    {
        ServiceInfo sv_info{};

        if (!App.getRedisManager().exists(token)) {
            sv_info.message["message"] = "Authentication failed, please login again";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        if (!App.getRedisManager().expire(token)) {
            sv_info.message["message"] = "Authentication  extend failed in [querySubmission]";
            sv_info.status = InternalServerError;
            return sv_info;
        }

        if (App.getRedisManager().exists("submission_tmp_" + submission_id)) {
            sv_info.status = OK;
            sv_info.message["message"] = "query succeed";
            sv_info.message["submission_status"] = App.getRedisManager().get("submission_tmp_" + submission_id);
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
