#include "tool_draw_point_2d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_point2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawPoint2D::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    m_temp_point = &add_entity<EntityPoint2D>();
    m_temp_point->m_selection_invisible = true;
    m_temp_point->m_p = get_cursor_pos_in_plane();
    m_temp_point->m_wrkpl = m_wrkpl->m_uuid;
    return ToolResponse();
}

bool ToolDrawPoint2D::can_begin()
{
    return get_workplane_uuid() != UUID();
}

glm::dvec2 ToolDrawPoint2D::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal_vector()));
}

ToolResponse ToolDrawPoint2D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_point) {
            m_temp_point->m_p = get_cursor_pos_in_plane();
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            m_temp_point->m_selection_invisible = false;
            if (m_constrain) {
                const EntityAndPoint pt{m_temp_point->m_uuid, 0};
                constrain_point(m_wrkpl->m_uuid, pt);
            }
            return ToolResponse::commit();
        } break;

        case InToolActionID::TOGGLE_CONSTRUCTION: {
            m_temp_point->m_construction = !m_temp_point->m_construction;
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
        update_tip();
    }

    return ToolResponse();
}

void ToolDrawPoint2D::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    actions.emplace_back(InToolActionID::LMB, "place point");

    actions.emplace_back(InToolActionID::RMB, "end tool");

    if (m_temp_point->m_construction)
        actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
    else
        actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");

    m_intf.tool_bar_set_tool_tip("");
    if (m_constrain) {
        set_constrain_tip("point");
    }
    m_intf.tool_bar_set_actions(actions);
}
} // namespace dune3d
