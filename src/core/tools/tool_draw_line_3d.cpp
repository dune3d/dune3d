#include "tool_draw_line_3d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "editor/editor_interface.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawLine3D::begin(const ToolArgs &args)
{
    update_tip();
    return ToolResponse();
}

void ToolDrawLine3D::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place point");
    if (m_temp_line) {
        actions.emplace_back(InToolActionID::RMB, "finish current segment");
    }
    else {
        actions.emplace_back(InToolActionID::RMB, "end tool");
    }
    m_intf.tool_bar_set_actions(actions);
}

ToolResponse ToolDrawLine3D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_line) {
            m_temp_line->m_p2 = m_intf.get_cursor_pos();
        }
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:

        {
            auto last_line = m_temp_line;
            m_temp_line = &add_entity<EntityLine3D>();
            m_temp_line->m_p1 = m_intf.get_cursor_pos();
            m_temp_line->m_p2 = m_intf.get_cursor_pos();
            if (last_line) {
                m_constraint = &add_constraint<ConstraintPointsCoincident>();
                m_constraint->m_entity1 = {last_line->m_uuid, 2};
                m_constraint->m_entity2 = {m_temp_line->m_uuid, 1};
            }
        }


            update_tip();
            break;

        case InToolActionID::RMB:
            if (m_temp_line) {
                m_core.get_current_document().m_entities.erase(m_temp_line->m_uuid);
                m_temp_line = nullptr;
                if (m_constraint)
                    m_core.get_current_document().m_constraints.erase(m_constraint->m_uuid);
            }
            else {

                return ToolResponse::commit();
            }
            break;


        case InToolActionID::CANCEL:
            if (m_temp_line) {
                m_core.get_current_document().m_entities.erase(m_temp_line->m_uuid);
                m_temp_line = nullptr;
                if (m_constraint)
                    m_core.get_current_document().m_constraints.erase(m_constraint->m_uuid);
            }
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
