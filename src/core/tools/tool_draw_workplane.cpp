#include "tool_draw_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "editor/editor_interface.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/action_label.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"


namespace dune3d {

ToolResponse ToolDrawWorkplane::begin(const ToolArgs &args)
{
    m_intf.enable_hover_selection();


    m_wrkpl = &add_entity<EntityWorkplane>();
    m_wrkpl->m_origin = m_intf.get_cursor_pos();
    m_wrkpl->m_normal = glm::dquat(1, 0, 0, 0);
    m_wrkpl->m_selection_invisible = true;

    update_tip();
    return ToolResponse();
}

void ToolDrawWorkplane::update_tip()
{
    m_intf.tool_bar_set_tool_tip("");
    std::vector<ConstraintType> constraint_icons;
    if (m_constrain) {
        set_constrain_tip("origin");
        update_constraint_icons(constraint_icons);
    }
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place workplane");

    actions.emplace_back(InToolActionID::RMB, "end tool");

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");

    m_intf.set_constraint_icons(m_intf.get_cursor_pos(), {NAN, NAN, NAN}, constraint_icons);

    m_intf.tool_bar_set_actions(actions);
}

ToolResponse ToolDrawWorkplane::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        m_wrkpl->m_origin = m_intf.get_cursor_pos();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_constrain) {
                const EntityAndPoint origin{m_wrkpl->m_uuid, 1};
                constrain_point({}, origin);
            }

            return ToolResponse::commit();

        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    update_tip();

    return ToolResponse();
}
} // namespace dune3d
