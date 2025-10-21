#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class ConstraintPointDistanceBase;
class EntitySTEP;

class ToolImportSTEP : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    CanBegin can_begin() override;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::TOGGLE_LOCK_ROTATION_CONSTRAINT, I::ROTATE_X, I::ROTATE_Y, I::ROTATE_Z,
        };
    }

private:
    EntitySTEP *m_step = nullptr;

    bool m_lock_rotation = false;

    void update_tip();
};
} // namespace dune3d
