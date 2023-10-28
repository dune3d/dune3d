#include "tool_draw_arc_2d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawArc2D::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    return ToolResponse();
}

bool ToolDrawArc2D::can_begin()
{
    return m_core.get_current_workplane() != UUID();
}

glm::dvec2 ToolDrawArc2D::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal()));
}

ToolResponse ToolDrawArc2D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_arc) {
            m_temp_arc->m_to = get_cursor_pos_in_plane();
            m_temp_arc->m_center = (m_temp_arc->m_to + m_temp_arc->m_from) / 2.;
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto pte =
                    entity_and_point_from_hover_selection(m_core.get_current_document(), m_intf.get_hover_selection());

            bool had_arc = m_temp_arc;
            if (!had_arc) {
                m_temp_arc = &add_entity<EntityArc2D>();
                m_temp_arc->m_selection_invisible = true;
                m_temp_arc->m_from = get_cursor_pos_in_plane();
                m_temp_arc->m_to = get_cursor_pos_in_plane();
                m_temp_arc->m_center = get_cursor_pos_in_plane();
                m_temp_arc->m_wrkpl = m_wrkpl->m_uuid;
            }


            if (pte.has_value()) {
                EntityArc2D *line = m_temp_arc;
                unsigned int pt = 1;
                if (had_arc)
                    pt = 2;
                auto pos = get_entity(pte->entity).get_point(pte->point, get_doc());

                if (pt == 1)
                    line->m_from = m_wrkpl->project(pos);
                else
                    line->m_to = m_wrkpl->project(pos);

                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = {line->m_uuid, pt};
                constraint.m_entity2 = *pte;
                constraint.m_wrkpl = m_wrkpl->m_uuid;
            }

            if (had_arc) {
                return ToolResponse::commit();
            }
        }

        break;

        case InToolActionID::RMB:


        case InToolActionID::CANCEL:

            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}

void ToolDrawArc2D::update_tip()
{
    auto pt = entity_and_point_from_hover_selection(get_doc(), m_intf.get_hover_selection());
    if (pt.has_value())
        m_intf.tool_bar_set_tool_tip("point");
    else
        m_intf.tool_bar_set_tool_tip("");
}


} // namespace dune3d
