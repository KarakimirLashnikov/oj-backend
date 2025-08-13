#pragma once
#include "Types.hpp"
#include "DbWorker.hpp"

namespace OJApp::DbOp
{
    enum class OpType
    {
        INSERT,
        UPDATE,
        DELETE,
        QUERY
    };

#define OPERATION_TYPE(op)                                                             \
    inline static OpType getStaticOpTypeEM() { return OpType::op; }                    \
    inline virtual OpType getOpTypeEM() const override { return getStaticOpTypeEM(); } \
    inline virtual const char *getOpTypeName() const override { return #op; }

    class DbOperation
    {
    public:
        DbOperation(std::string sql, njson param_array);
        virtual ~DbOperation() noexcept = default;

        virtual void operator()(std::shared_ptr<Database::DbWorker>);

        virtual OpType getOpTypeEM() const = 0;
        virtual const char *getOpTypeName() const = 0;

    protected:
        void setSqlVariables(sql::PreparedStatement *pstmt);

    protected:
        std::string m_Sql;
        njson m_ParamArr{};
    };
}