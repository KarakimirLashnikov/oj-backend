#include "services/ProblemService.hpp"
#include "dbop/DbOpFactory.hpp"
#include "Application.hpp"

namespace OJApp
{
    using DbOp::makeDbOp;
    using DbOp::OpType;

    using Core::Types::difficultyToString;
    ServiceInfo ProblemService::createProblem(ProblemInfo info, const std::string& token)
    {
        ServiceInfo sv_info{};
        
        if (!App.getRedisManager().exists(token)) {
            sv_info.message = "Authentication failed, please login again";
            sv_info.status = Unauthorized;
            return sv_info;
        }

        if (!App.getRedisManager().expire(token)) {
            sv_info.message = "Authentication  extend failed in [createProblem]";
            sv_info.status = InternalServerError;
            return sv_info;
        }

        auto db_op = makeDbOp(OpType::Query, R"SQL(SELECT * FROM problems p WHERE p.title = ?)SQL", { info.title });
        njson::array_t result = App.getDbManager().query(dynamic_cast<DbOp::DbQueryOp *>(db_op.get()));
        if (!result.empty()) {
            sv_info.status = Conflict;
            sv_info.message = "Problems's title is used.";
            return sv_info;
        }

        std::string username{ App.getRedisManager().get(token).value() };
        DbOperateMessage msg{
            .op_type = OpType::Insert,
            .sql = R"SQL(WITH u AS (SELECT id FROM users WHERE username = ? LIMIT 1)
INSERT INTO problems (title,difficulty,description,author_id) 
SELECT ?, ?, ?, id FROM u;)SQL",
            .data_key = info.title,
            .param_array = { username, info.title,difficultyToString(info.difficulty),info.description }
        };
        App.getRedisManager().publishDbOperate(msg);

        sv_info.status = Created;
        sv_info.message = "Create problems successfully";
        return sv_info;
    }
}