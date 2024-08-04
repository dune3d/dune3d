#include "tool_common.hpp"
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"


namespace dune3d {

class EntityText;
class EnterTextWindow;

class ToolDrawText : public virtual ToolCommon, private ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

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
        };
    }

    CanBegin can_begin() override;

private:
    EntityText *m_entity = nullptr;
    EnterTextWindow *m_win = nullptr;

    void update_tip();
    bool m_constrain = true;
    const EntityWorkplane *m_wrkpl;
};
} // namespace dune3d
