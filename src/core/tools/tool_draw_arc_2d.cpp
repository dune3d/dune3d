#include "tool_draw_arc_2d.hpp"
#include "document/document.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "util/glm_util.hpp"
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
    return get_workplane_uuid() != UUID();
}

glm::dvec2 ToolDrawArc2D::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal()));
}

EntityAndPoint ToolDrawArc2D::get_entity_and_point(State state, bool flipped) const
{
    switch (state) {
    case State::FROM:
        if (!flipped)
            return {m_temp_arc->m_uuid, 1};
        else
            return {m_temp_arc->m_uuid, 2};
    case State::TO:
        if (!flipped)
            return {m_temp_arc->m_uuid, 2};
        else
            return {m_temp_arc->m_uuid, 1};
    case State::CENTER:
        return {m_temp_arc->m_uuid, 3};
    }
}

ToolResponse ToolDrawArc2D::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_arc) {
            if (m_state == State::TO) {
                if (!m_flipped)
                    m_temp_arc->m_to = get_cursor_pos_in_plane();
                else
                    m_temp_arc->m_from = get_cursor_pos_in_plane();
                m_temp_arc->m_center = (m_temp_arc->m_to + m_temp_arc->m_from) / 2.;
            }
            else { // CENTER
                m_temp_arc->m_center =
                        project_onto_perp_bisector(m_temp_arc->m_from, m_temp_arc->m_to, get_cursor_pos_in_plane());
            }
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            switch (m_state) {
            case State::FROM:
                m_temp_arc = &add_entity<EntityArc2D>();
                m_temp_arc->m_selection_invisible = true;
                m_temp_arc->m_from = get_cursor_pos_in_plane();
                m_temp_arc->m_to = get_cursor_pos_in_plane() + glm::dvec2(2, 0); // so that the solver is happy
                m_temp_arc->m_center = get_cursor_pos_in_plane() + glm::dvec2(1, 0);
                m_temp_arc->m_wrkpl = m_wrkpl->m_uuid;
                if (m_constrain) {
                    m_constraint_from = constrain_point(m_wrkpl->m_uuid, get_entity_and_point(m_state, m_flipped));
                }
                m_temp_arc->m_to = m_temp_arc->m_from;
                m_temp_arc->m_center = m_temp_arc->m_from;
                m_state = State::TO;
                break;

            case State::TO:
                if (m_constrain) {
                    m_constraint_to = constrain_point(m_wrkpl->m_uuid, get_entity_and_point(m_state, m_flipped));
                }
                m_state = State::CENTER;
                break;

            case State::CENTER:
                if (m_constrain) {
                    constrain_point(m_wrkpl->m_uuid, get_entity_and_point(m_state, m_flipped));
                }
                return ToolResponse::commit();
            }
        } break;

        case InToolActionID::TOGGLE_CONSTRUCTION: {
            if (m_temp_arc)
                m_temp_arc->m_construction = !m_temp_arc->m_construction;
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::FLIP_ARC: {
            if (m_temp_arc) {
                std::swap(m_temp_arc->m_from, m_temp_arc->m_to);
                if (m_constraint_from) {
                    m_constraint_from->replace_point(get_entity_and_point(State::FROM, m_flipped),
                                                     get_entity_and_point(State::FROM, !m_flipped));
                }
                if (m_constraint_to) {
                    m_constraint_to->replace_point(get_entity_and_point(State::TO, m_flipped),
                                                   get_entity_and_point(State::TO, !m_flipped));
                }
                m_flipped = !m_flipped;
            }
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

void ToolDrawArc2D::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    switch (m_state) {
    case State::FROM:
        actions.emplace_back(InToolActionID::LMB, "place from");
        break;
    case State::TO:
        actions.emplace_back(InToolActionID::LMB, "place to");
        break;
    case State::CENTER:
        actions.emplace_back(InToolActionID::LMB, "place center");
        break;
    }
    actions.emplace_back(InToolActionID::RMB, "end tool");

    switch (m_state) {
    case State::FROM:
        break;
    case State::TO:
    case State::CENTER:
        actions.emplace_back(InToolActionID::FLIP_ARC, "flip arc");
        break;
    }

    if (m_temp_arc) {
        if (m_temp_arc->m_construction)
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
        else
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");
    }

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");


    m_intf.tool_bar_set_tool_tip("");
    if (m_constrain) {
        switch (m_state) {
        case State::FROM:
            set_constrain_tip("from");
            break;
        case State::TO:
            set_constrain_tip("to");
            break;
        case State::CENTER:
            set_constrain_tip("center");
            break;
        }
    }
    m_intf.tool_bar_set_actions(actions);
}


} // namespace dune3d
