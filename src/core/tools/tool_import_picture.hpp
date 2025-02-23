#include "tool_common.hpp"
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class EntityPicture;

class ToolImportPicture : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;

    CanBegin can_begin() override;
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


private:
    EntityPicture *m_pic = nullptr;
    const EntityWorkplane *m_wrkpl = nullptr;

    void update_tip();
    bool m_constrain = true;
};
} // namespace dune3d
