#include "tool_draw_rectangle.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_point2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_hv.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolResponse ToolDrawRectangle::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    m_lines = {nullptr};
    return ToolResponse();
}

ToolBase::CanBegin ToolDrawRectangle::can_begin()
{
    return get_workplane_uuid() != UUID();
}

glm::dvec2 ToolDrawRectangle::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl));
}

ToolResponse ToolDrawRectangle::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_lines.front()) {
            glm::dvec2 pa;
            glm::dvec2 pb = get_cursor_pos_in_plane();

            if (m_mode == Mode::CORNER) {
                pa = m_first_point;
            }
            else {
                auto d = get_cursor_pos_in_plane() - m_first_point;
                pa = m_first_point - d;
            }
            const auto p1 = pa;
            const auto p2 = glm::dvec2(pb.x, pa.y);
            const auto p3 = pb;
            const auto p4 = glm::dvec2(pa.x, pb.y);
            m_lines.at(0)->m_p1 = p1;
            m_lines.at(0)->m_p2 = p2;
            m_lines.at(1)->m_p1 = p2;
            m_lines.at(1)->m_p2 = p3;
            m_lines.at(2)->m_p1 = p3;
            m_lines.at(2)->m_p2 = p4;
            m_lines.at(3)->m_p1 = p4;
            m_lines.at(3)->m_p2 = p1;
        }
        update_tip();
        m_intf.set_first_update_group(m_core.get_current_group());
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_lines.front()) {
                auto last_line = m_lines.back();
                size_t i = 0;
                for (auto line : m_lines) {
                    {
                        auto &constraint = add_constraint<ConstraintPointsCoincident>();
                        constraint.m_entity1 = {last_line->m_uuid, 2};
                        constraint.m_entity2 = {line->m_uuid, 1};
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                    }
                    {
                        ConstraintHV *constraint = nullptr;
                        if (i == 0 || i == 2)
                            constraint = &add_constraint<ConstraintHorizontal>();
                        else
                            constraint = &add_constraint<ConstraintVertical>();
                        constraint->m_entity1 = {line->m_uuid, 1};
                        constraint->m_entity2 = {line->m_uuid, 2};
                        constraint->m_wrkpl = m_wrkpl->m_uuid;
                    }

                    last_line = line;
                    i++;
                }
                if (m_constrain) {
                    constrain_point(m_wrkpl->m_uuid, {m_lines.at(2)->m_uuid, 1});
                }
                if (m_mode == Mode::CORNER) {
                    if (m_first_constraint)
                        constrain_point(m_first_constraint.value(), m_wrkpl->m_uuid, m_first_enp,
                                        {m_lines.at(0)->m_uuid, 1});
                }
                else {
                    auto &diagonal = add_entity<EntityLine2D>();
                    diagonal.m_wrkpl = m_wrkpl->m_uuid;
                    diagonal.m_construction = true;
                    diagonal.m_p1 = m_lines.at(0)->m_p1;
                    diagonal.m_p2 = m_lines.at(1)->m_p2;
                    {
                        auto &constraint = add_constraint<ConstraintPointsCoincident>();
                        constraint.m_entity1 = {diagonal.m_uuid, 1};
                        constraint.m_entity2 = {m_lines.at(0)->m_uuid, 1};
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                    }
                    {
                        auto &constraint = add_constraint<ConstraintPointsCoincident>();
                        constraint.m_entity1 = {diagonal.m_uuid, 2};
                        constraint.m_entity2 = {m_lines.at(1)->m_uuid, 2};
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                    }
                    if (m_first_constraint && *m_first_constraint == Constraint::Type::POINTS_COINCIDENT) {
                        auto &constraint = add_constraint<ConstraintMidpoint>();
                        constraint.m_line = diagonal.m_uuid;
                        constraint.m_point = m_first_enp;
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                    }
                    else {
                        auto &midpt = add_entity<EntityPoint2D>();
                        midpt.m_wrkpl = m_wrkpl->m_uuid;
                        midpt.m_construction = true;
                        midpt.m_p = (diagonal.m_p1 + diagonal.m_p2) / 2.;
                        {
                            auto &constraint = add_constraint<ConstraintMidpoint>();
                            constraint.m_line = diagonal.m_uuid;
                            constraint.m_point = {midpt.m_uuid, 0};
                            constraint.m_wrkpl = m_wrkpl->m_uuid;
                        }
                        if (m_first_constraint)
                            constrain_point(m_first_constraint.value(), m_wrkpl->m_uuid, m_first_enp,
                                            {midpt.m_uuid, 0});
                    }
                }
                return ToolResponse::commit();
            }
            else {
                m_first_point = get_cursor_pos_in_plane();
                for (auto &it : m_lines) {
                    it = &add_entity<EntityLine2D>();
                    it->m_selection_invisible = true;
                    it->m_wrkpl = m_wrkpl->m_uuid;
                    it->m_p1 = m_first_point;
                    it->m_p2 = m_first_point;
                }

                if (m_constrain) {
                    m_first_constraint = get_constraint_type();
                    if (auto hsel = m_intf.get_hover_selection())
                        m_first_enp = hsel->get_entity_and_point();
                }

                return ToolResponse();
            }
        } break;

        case InToolActionID::TOGGLE_CONSTRUCTION: {
            if (m_lines.front()) {
                m_lines.front()->m_construction = !m_lines.front()->m_construction;
                for (auto it : m_lines) {
                    it->m_construction = m_lines.front()->m_construction;
                }
            }
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::TOGGLE_RECTANGLE_MODE: {
            if (m_mode == Mode::CENTER)
                m_mode = Mode::CORNER;
            else
                m_mode = Mode::CENTER;
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

void ToolDrawRectangle::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    std::string what;
    if (m_lines.front())
        what = "corner";
    else if (m_mode == Mode::CORNER)
        what = "corner";
    else
        what = "center";

    actions.emplace_back(InToolActionID::LMB, "place " + what);

    actions.emplace_back(InToolActionID::RMB, "end tool");

    if (m_lines.front()) {
        if (m_lines.front()->m_construction)
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
        else
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");
    }

    if (m_mode == Mode::CENTER)
        actions.emplace_back(InToolActionID::TOGGLE_RECTANGLE_MODE, "from corner");
    else
        actions.emplace_back(InToolActionID::TOGGLE_RECTANGLE_MODE, "from center");


    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");


    m_intf.tool_bar_set_tool_tip("");
    std::vector<ConstraintType> constraint_icons;

    if (m_constrain) {
        set_constrain_tip(what);
        update_constraint_icons(constraint_icons);
    }
    m_intf.tool_bar_set_actions(actions);
    m_intf.set_constraint_icons(get_cursor_pos_for_workplane(*m_wrkpl), m_wrkpl->transform_relative({1, 1}),
                                constraint_icons);
}
} // namespace dune3d
