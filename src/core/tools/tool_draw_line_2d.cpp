#include "tool_draw_line_2d.hpp"
#include "document/document.hpp"
#include "document/entity_line2d.hpp"
#include "document/entity_workplane.hpp"
#include "document/constraint_points_coincident.hpp"
#include "editor_interface.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawLine2D::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    return ToolResponse();
}

bool ToolDrawLine2D::can_begin()
{
    return m_core.get_current_workplane() != UUID();
}

glm::dvec2 ToolDrawLine2D::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal()));
}

ToolResponse ToolDrawLine2D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_line) {
            m_temp_line->m_p2 = get_cursor_pos_in_plane();
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto pte = entity_and_point_from_hover_selection(get_doc(), m_intf.get_hover_selection());

            auto last_line = m_temp_line;
            m_temp_line = &add_entity<EntityLine2D>();
            m_temp_line->m_selection_invisible = true;
            m_temp_line->m_p1 = get_cursor_pos_in_plane();
            m_temp_line->m_p2 = get_cursor_pos_in_plane();
            m_temp_line->m_wrkpl = m_wrkpl->m_uuid;

            if (pte.has_value()) {
                EntityLine2D *line = last_line ? last_line : m_temp_line;
                unsigned int pt = 1;
                if (last_line)
                    pt = 2;
                auto pos = get_entity(pte->entity).get_point(pte->point, get_doc());

                if (pt == 1)
                    line->m_p1 = m_wrkpl->project(pos);
                else
                    line->m_p2 = m_wrkpl->project(pos);

                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = {line->m_uuid, pt};
                constraint.m_entity2 = *pte;
                constraint.m_wrkpl = m_wrkpl->m_uuid;
                m_constraint = &constraint;
            }

            if (last_line) {
                last_line->m_selection_invisible = false;
                m_constraint = &add_constraint<ConstraintPointsCoincident>();
                m_constraint->m_entity1 = {last_line->m_uuid, 2};
                m_constraint->m_entity2 = {m_temp_line->m_uuid, 1};
                m_constraint->m_wrkpl = m_wrkpl->m_uuid;
            }
        }

        break;

        case InToolActionID::RMB:
            if (m_temp_line) {
                get_doc().m_entities.erase(m_temp_line->m_uuid);
                m_temp_line = nullptr;
                if (m_constraint)
                    get_doc().m_constraints.erase(m_constraint->m_uuid);
            }
            else {
                return ToolResponse::commit();
            }
            break;


        case InToolActionID::CANCEL:
            if (m_temp_line) {
                get_doc().m_entities.erase(m_temp_line->m_uuid);
                m_temp_line = nullptr;
                if (m_constraint)
                    get_doc().m_constraints.erase(m_constraint->m_uuid);
            }
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}

void ToolDrawLine2D::update_tip()
{
    auto pt = entity_and_point_from_hover_selection(m_core.get_current_document(), m_intf.get_hover_selection());
    if (pt.has_value())
        m_intf.tool_bar_set_tool_tip("point");
    else
        m_intf.tool_bar_set_tool_tip("");
}


} // namespace dune3d
