#include "tool_draw_circle_2d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawCircle2D::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
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
                if (m_constrain) {
                    if (auto hsel = m_intf.get_hover_selection()) {
                        if (hsel->type == SelectableRef::Type::ENTITY) {
                            const auto enp = hsel->get_entity_and_point();
                            if (get_doc().is_valid_point(enp)) {
                                auto &constraint = add_constraint<ConstraintPointOnCircle>();
                                constraint.m_circle = m_temp_circle->m_uuid;
                                constraint.m_point = enp;
                                constraint.m_wrkpl = m_wrkpl->m_uuid;
                                constraint.m_modify_to_satisfy = true;
                            }
                        }
                    }
                }
                return ToolResponse::commit();
            }
            else {
                m_temp_circle = &add_entity<EntityCircle2D>();
                m_temp_circle->m_selection_invisible = true;
                m_temp_circle->m_radius = 0;
                m_temp_circle->m_center = get_cursor_pos_in_plane();
                m_temp_circle->m_wrkpl = m_wrkpl->m_uuid;

                if (m_constrain) {
                    const EntityAndPoint circle_center{m_temp_circle->m_uuid, 1};
                    constrain_point(m_wrkpl->m_uuid, circle_center);
                }

                return ToolResponse();
            }
        } break;

        case InToolActionID::TOGGLE_CONSTRUCTION: {
            if (m_temp_circle)
                m_temp_circle->m_construction = !m_temp_circle->m_construction;
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

void ToolDrawCircle2D::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    if (m_temp_circle)
        actions.emplace_back(InToolActionID::LMB, "place radius");
    else
        actions.emplace_back(InToolActionID::LMB, "place center");

    actions.emplace_back(InToolActionID::RMB, "end tool");

    if (m_temp_circle) {
        if (m_temp_circle->m_construction)
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
        else
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");
    }

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");


    m_intf.tool_bar_set_tool_tip("");
    if (m_constrain && !m_temp_circle) {
        set_constrain_tip("center");
    }
    if (m_constrain && m_temp_circle) {
        if (auto hsel = m_intf.get_hover_selection()) {
            if (hsel->type == SelectableRef::Type::ENTITY) {
                const auto enp = hsel->get_entity_and_point();
                if (get_doc().is_valid_point(enp)) {
                    m_intf.tool_bar_set_tool_tip("constrain radius on point");
                }
            }
        }
    }
    m_intf.tool_bar_set_actions(actions);
}
} // namespace dune3d
