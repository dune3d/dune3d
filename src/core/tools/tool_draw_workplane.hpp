#include "tool_common.hpp"
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class ToolDrawWorkplane : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    CanBegin can_begin() override;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::TOGGLE_COINCIDENT_CONSTRAINT,
                I::ROTATE_X,
                I::ROTATE_Y,
                I::ROTATE_Z,
                I::TOGGLE_LOCK_ROTATION_CONSTRAINT,
                I::TOGGLE_AUTO_NORMAL,
        };
    }

    bool handles_view_changed() const override
    {
        return true;
    }

private:
    class EntityWorkplane *m_wrkpl = nullptr;

    bool m_constrain = true;
    bool m_lock_rotation = false;
    bool m_auto_normal = true;

    void update_tip();
};
} // namespace dune3d
