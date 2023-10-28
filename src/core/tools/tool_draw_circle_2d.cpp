#include "tool_draw_circle_2d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "editor_interface.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawCircle2D::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    return ToolResponse();
}

bool ToolDrawCircle2D::can_begin()
{
    return m_core.get_current_workplane() != UUID();
}

glm::dvec2 ToolDrawCircle2D::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal()));
}

ToolResponse ToolDrawCircle2D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_circle) {
            m_temp_circle->m_radius = glm::length(get_cursor_pos_in_plane() - m_temp_circle->m_center);
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {

            if (m_temp_circle) {
                m_temp_circle->m_selection_invisible = false;
                return ToolResponse::commit();
            }
            else {
                m_temp_circle = &add_entity<EntityCircle2D>();
                m_temp_circle->m_selection_invisible = true;
                m_temp_circle->m_radius = 0;
                m_temp_circle->m_center = get_cursor_pos_in_plane();
                m_temp_circle->m_wrkpl = m_wrkpl->m_uuid;
                return ToolResponse();
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

void ToolDrawCircle2D::update_tip()
{
}


} // namespace dune3d
