#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <map>

namespace dune3d {

class Group;

class ToolMove : public ToolCommon {
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
        };
    }
    bool is_specific() override
    {
        return true;
    }
    CanBegin can_begin() override;

    bool needs_delayed_begin() const override
    {
        return true;
    }


private:
    glm::dvec3 m_inital_pos;
    std::map<UUID, glm::dvec2> m_inital_pos_wrkpl;
    std::map<UUID, glm::dvec3> m_inital_pos_angle_constraint;
    UUID m_first_group;
    std::set<std::pair<Entity *, unsigned int>> m_entities;
    ICore::DraggedList m_dragged_list;
};
} // namespace dune3d
