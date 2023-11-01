#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class EntityWorkplane;
class Entity;
class ConstraintWorkplaneNormal;

class ToolConstrainWorkplaneNormal : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

private:
    EntityWorkplane *m_wrkpl = nullptr;
    EntityWorkplane *get_wrkpl();
    Entity *m_line1 = nullptr;
    ConstraintWorkplaneNormal *m_constraint = nullptr;
    void update_tip();
};
} // namespace dune3d
