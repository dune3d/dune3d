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
    m_intf.enable_hover_selection();
    return ToolResponse();
}

void ToolDrawLine3D::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(9);
    actions.emplace_back(InToolActionID::LMB, "place point");
    actions.emplace_back(InToolActionID::RMB, "end tool");

    if (m_temp_line) {
        if (m_temp_line->m_construction)
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
        else
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");
    }

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "coincident constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "coincident constraint on");

    m_intf.tool_bar_set_actions(actions);

    std::vector<ConstraintType> constraint_icons;

    if (m_constrain) {
        std::string what = m_temp_line ? "to" : "from";
        set_constrain_tip(what);
        update_constraint_icons(constraint_icons);
    }
    else {
        m_intf.tool_bar_set_tool_tip("");
    }

    glm::vec3 v = {NAN, NAN, NAN};
    if (m_temp_line) {
        v = m_temp_line->m_p2 - m_temp_line->m_p1;
    }

    m_intf.set_constraint_icons(m_intf.get_cursor_pos(), v, constraint_icons);
}


ToolResponse ToolDrawLine3D::end_tool()
{
    if (m_temp_line) {
        m_core.get_current_document().m_entities.erase(m_temp_line->m_uuid);
        m_temp_line = nullptr;
        if (m_constraint)
            m_core.get_current_document().m_constraints.erase(m_constraint->m_uuid);
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
    return ToolResponse();
}

ToolResponse ToolDrawLine3D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_line) {
            m_temp_line->m_p2 = m_intf.get_cursor_pos();
        }

        set_first_update_group_current();
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_constrain && m_temp_line) {
                if (constrain_point(get_workplane_uuid(), {m_temp_line->m_uuid, 2})) {
                    return ToolResponse::commit();
                }
            }

            if (m_temp_line && m_last == m_intf.get_cursor_pos())
                return end_tool();

            auto last_line = m_temp_line;

            m_temp_line = &add_entity<EntityLine3D>();
            m_temp_line->m_p1 = m_intf.get_cursor_pos();
            m_temp_line->m_p2 = m_intf.get_cursor_pos();
            m_temp_line->m_selection_invisible = true;
            m_temp_lines.push_back(m_temp_line);
            if (last_line) {
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                constraint.m_entity1 = {last_line->m_uuid, 2};
                constraint.m_entity2 = {m_temp_line->m_uuid, 1};
                m_constraint = &constraint;
            }
            update_tip();


            if (m_constrain) {
                if (!last_line) {
                    if (auto constraint = constrain_point(get_workplane_uuid(), {m_temp_line->m_uuid, 1})) {
                        m_constraint = constraint;
                    }
                }
            }

            m_last = m_intf.get_cursor_pos();
        } break;


        case InToolActionID::TOGGLE_CONSTRUCTION: {
            if (m_temp_line) {
                m_temp_line->m_construction = !m_temp_line->m_construction;
                for (auto en : m_temp_lines) {
                    en->m_construction = m_temp_line->m_construction;
                }
            }
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL: {
            auto r = end_tool();
            if (r.result != ToolResponse::Result::NOP)
                return r;
        } break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace dune3d
