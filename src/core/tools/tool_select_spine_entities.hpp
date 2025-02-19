#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class GroupPipe;

class ToolSelectSpineEntities : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return false;
    }
    CanBegin can_begin() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::CLEAR_SPINE_ENTITIES,
        };
    }

private:
    GroupPipe *m_group = nullptr;

    void update_selection();
};
} // namespace dune3d
