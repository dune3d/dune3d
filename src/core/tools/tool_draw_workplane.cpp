#include "tool_draw_workplane.hpp"
#include "document/document.hpp"
#include "document/entity_workplane.hpp"
#include "editor_interface.hpp"
#include "document/constraint_points_coincident.hpp"
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
    auto pt = entity_and_point_from_hover_selection(get_doc(), m_intf.get_hover_selection());
    if (pt.has_value())
        m_intf.tool_bar_set_tool_tip("point");
    else
        m_intf.tool_bar_set_tool_tip("");
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place workplane");

    actions.emplace_back(InToolActionID::RMB, "end tool");

    m_intf.tool_bar_set_actions(actions);
}

ToolResponse ToolDrawWorkplane::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        m_wrkpl->m_origin = m_intf.get_cursor_pos();
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto pte = entity_and_point_from_hover_selection(get_doc(), m_intf.get_hover_selection());
            if (pte.has_value()) {
                m_wrkpl->m_origin = get_doc().get_point(*pte);
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = {m_wrkpl->m_uuid, 1};
                constraint.m_entity2 = *pte;
            }


            return ToolResponse::commit();

        } break;


        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
