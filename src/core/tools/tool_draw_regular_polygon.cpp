#include "tool_draw_regular_polygon.hpp"
#include "document/document.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "document/constraint/constraint_equal_length.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "tool_common_impl.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_datum_window.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

namespace dune3d {

ToolResponse ToolDrawRegularPolygon::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    return ToolResponse();
}

ToolBase::CanBegin ToolDrawRegularPolygon::can_begin()
{
    return get_workplane_uuid() != UUID();
}

glm::dvec2 ToolDrawRegularPolygon::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl));
}

void ToolDrawRegularPolygon::set_n_sides(unsigned int n)
{
    for (auto it : m_sides) {
        get_doc().m_entities.erase(it->m_uuid);
    }
    m_sides.clear();
    for (unsigned int i = 0; i < n; i++) {
        auto &it = add_entity<EntityLine2D>();
        it.m_wrkpl = m_wrkpl->m_uuid;
        it.m_selection_invisible = true;
        m_sides.push_back(&it);
    }
    update_sides(get_cursor_pos_in_plane());
    update_tip();
}

void ToolDrawRegularPolygon::update_sides(const glm::dvec2 &p)
{
    if (!m_temp_circle)
        return;
    const auto center = m_temp_circle->m_center;
    const auto vstart = p - center;
    m_temp_circle->m_radius = glm::length(vstart);
    // const auto phistart = atan2(vstart.y, vstart.x);
    const auto phidelta = M_PI * 2. / m_sides.size();
    for (unsigned int i = 0; i < m_sides.size(); i++) {
        const auto pos1 = center + glm::rotate(vstart, phidelta * i);
        const auto pos2 = center + glm::rotate(vstart, phidelta * (i + 1));
        m_sides.at(i)->m_p1 = pos1;
        m_sides.at(i)->m_p2 = pos2;
    }
}

ToolResponse ToolDrawRegularPolygon::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_circle) {
            m_temp_circle->m_radius = glm::length(get_cursor_pos_in_plane() - m_temp_circle->m_center);
            update_sides(get_cursor_pos_in_plane());
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_temp_circle) {
                auto last_line = m_sides.back();
                for (auto line : m_sides) {
                    {
                        auto &constraint = add_constraint<ConstraintPointOnCircle>();
                        constraint.m_circle = m_temp_circle->m_uuid;
                        constraint.m_point = {line->m_uuid, 1};
                        constraint.m_modify_to_satisfy = true;
                    }
                    if (line != m_sides.front()) {
                        auto &constraint = add_constraint<ConstraintEqualLength>();
                        constraint.m_entity1 = m_sides.front()->m_uuid;
                        constraint.m_entity2 = line->m_uuid;
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                        constraint.m_modify_to_satisfy = true;
                    }
                    {
                        auto &constraint = add_constraint<ConstraintPointsCoincident>();
                        constraint.m_entity1 = {last_line->m_uuid, 2};
                        constraint.m_entity2 = {line->m_uuid, 1};
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                    }
                    last_line = line;
                }

                if (m_constrain) {
                    if (auto hsel = m_intf.get_hover_selection()) {
                        if (hsel->type == SelectableRef::Type::ENTITY) {
                            const auto enp = hsel->get_entity_and_point();
                            if (get_doc().is_valid_point(enp)) {
                                auto &constraint = add_constraint<ConstraintPointOnCircle>();
                                constraint.m_circle = m_temp_circle->m_uuid;
                                constraint.m_point = enp;
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
                m_temp_circle->m_construction = true;
                m_temp_circle->m_radius = 0;
                m_temp_circle->m_center = get_cursor_pos_in_plane();
                m_temp_circle->m_wrkpl = m_wrkpl->m_uuid;
                set_n_sides(6);

                if (m_constrain) {
                    const EntityAndPoint circle_center{m_temp_circle->m_uuid, 1};
                    constrain_point(m_wrkpl->m_uuid, circle_center);
                }

                return ToolResponse();
            }
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::N_SIDES_DEC: {
            if (m_sides.size() > 3 && !m_win)
                set_n_sides(m_sides.size() - 1);

        } break;

        case InToolActionID::N_SIDES_INC: {
            if (!m_win)
                set_n_sides(m_sides.size() + 1);
        } break;

        case InToolActionID::ENTER_N_SIDES: {
            m_last_sides = m_sides.size();
            m_win = m_intf.get_dialogs().show_enter_datum_window("Enter sides", DatumUnit::INTEGER, m_sides.size());
            m_win->set_range(3, 30);
            m_win->set_step_size(1);
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
        update_tip();
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterDatumWindow *>(args.data.get())) {
                    set_n_sides(d->value);
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                m_win->close();
                m_win = nullptr;
                update_tip();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                if (m_win)
                    set_n_sides(m_last_sides);
                m_win = nullptr;
                update_tip();
            }
        }
    }

    return ToolResponse();
}

void ToolDrawRegularPolygon::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    if (m_temp_circle)
        actions.emplace_back(InToolActionID::LMB, "place radius");
    else
        actions.emplace_back(InToolActionID::LMB, "place center");

    actions.emplace_back(InToolActionID::RMB, "end tool");


    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "constraint on");

    if (m_temp_circle) {
        actions.emplace_back(InToolActionID::ENTER_N_SIDES);
        if (!m_win)
            actions.emplace_back(InToolActionID::N_SIDES_INC, InToolActionID::N_SIDES_DEC, "sides");
    }
    std::vector<ConstraintType> constraint_icons;
    glm::vec3 v = {NAN, NAN, NAN};
    auto tip = std::to_string(m_sides.size()) + " sides ";
    if (m_constrain && !m_temp_circle) {
        tip += get_constrain_tip("center");
        update_constraint_icons(constraint_icons);
    }
    if (m_constrain && m_temp_circle) {
        if (auto hsel = m_intf.get_hover_selection()) {
            if (hsel->type == SelectableRef::Type::ENTITY) {
                const auto enp = hsel->get_entity_and_point();
                if (get_doc().is_valid_point(enp)) {
                    const auto r = get_cursor_pos_in_plane() - m_temp_circle->m_center;
                    v = m_wrkpl->transform_relative({-r.y, r.x});
                    constraint_icons.push_back(Constraint::Type::POINT_ON_CIRCLE);
                    tip += " constrain radius on point";
                }
            }
        }
    }
    m_intf.tool_bar_set_tool_tip(tip);
    m_intf.tool_bar_set_actions(actions);
    m_intf.set_constraint_icons(get_cursor_pos_for_workplane(*m_wrkpl), v, constraint_icons);
}
} // namespace dune3d
