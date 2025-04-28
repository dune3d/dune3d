#include "tool_helper_constrain.hpp"
#include "editor/editor_interface.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "document/constraint/constraint_point_on_bezier.hpp"
#include "tool_common_impl.hpp"
#include <iostream>

namespace dune3d {

Constraint *ToolHelperConstrain::constrain_point(Constraint::Type type, const UUID &wrkpl, const EntityAndPoint &enp,
                                                 const EntityAndPoint &enp_to_constrain)
{
    switch (type) {
    case Constraint::Type::POINTS_COINCIDENT: {
        auto &constraint = add_constraint<ConstraintPointsCoincident>();
        constraint.m_entity1 = enp;
        constraint.m_entity2 = enp_to_constrain;
        constraint.m_wrkpl = wrkpl;
        m_core.solve_current();
        return &constraint;
    }
    case Constraint::Type::POINT_ON_LINE: {
        auto &constraint = add_constraint<ConstraintPointOnLine>();
        constraint.m_line = enp.entity;
        constraint.m_point = enp_to_constrain;
        constraint.m_wrkpl = wrkpl;
        constraint.m_modify_to_satisfy = true;
        m_core.solve_current();
        return &constraint;
    }
    case Constraint::Type::MIDPOINT: {
        auto &constraint = add_constraint<ConstraintMidpoint>();
        constraint.m_line = enp.entity;
        constraint.m_point = enp_to_constrain;
        constraint.m_wrkpl = wrkpl;
        constraint.m_modify_to_satisfy = true;
        m_core.solve_current();
        return &constraint;
    }
    case Constraint::Type::POINT_ON_CIRCLE: {
        auto &constraint = add_constraint<ConstraintPointOnCircle>();
        constraint.m_circle = enp.entity;
        constraint.m_point = enp_to_constrain;
        constraint.m_modify_to_satisfy = true;
        m_core.solve_current();
        return &constraint;
    }
    case Constraint::Type::POINT_ON_BEZIER: {
        auto &constraint = add_constraint<ConstraintPointOnBezier>();
        constraint.m_line = enp.entity;
        constraint.m_point = enp_to_constrain;
        constraint.m_wrkpl = get_workplane_uuid();
        constraint.modify_to_satisfy(get_doc());
        m_core.solve_current();
        return &constraint;
    }
    default:
        return nullptr;
    }
}

Constraint *ToolHelperConstrain::constrain_point(const UUID &wrkpl, const EntityAndPoint &enp_to_constrain)
{
    const auto ct = get_constraint_type();
    if (!ct)
        return nullptr;
    auto hsel = m_intf.get_hover_selection();
    const auto enp = hsel->get_entity_and_point();

    return constrain_point(*ct, wrkpl, enp, enp_to_constrain);
}

std::optional<Constraint::Type> ToolHelperConstrain::get_constraint_type()
{
    if (auto hsel = m_intf.get_hover_selection()) {
        if (hsel->is_entity()) {
            const auto enp = hsel->get_entity_and_point();
            if (get_doc().is_valid_point(enp)) {
                return Constraint::Type::POINTS_COINCIDENT;
            }
            else if (enp.point == 0) {
                using ET = Entity::Type;
                auto &entity = get_entity(enp.entity);
                if (entity.of_type(ET::LINE_2D, ET::LINE_3D)) {
                    const auto p2 = entity.get_point(2, get_doc());
                    const auto p1 = entity.get_point(1, get_doc());
                    const auto v = p2 - p1;
                    const auto u = glm::cross(v, glm::dvec3(m_intf.get_cam_normal()));
                    const auto n = glm::cross(u, v);
                    const auto pos = m_intf.get_cursor_pos_for_plane(p1, n);
                    const auto d = glm::length(pos - p1) / glm::length(v);
                    if (std::abs(d - 0.5) < .05)
                        return Constraint::Type::MIDPOINT;
                    return Constraint::Type::POINT_ON_LINE;
                }
                else if (entity.of_type(ET::ARC_2D, ET::CIRCLE_2D, ET::ARC_3D, ET::CIRCLE_3D)) {
                    return Constraint::Type::POINT_ON_CIRCLE;
                }
                else if (entity.of_type(ET::BEZIER_2D, ET::BEZIER_3D)) {
                    if (get_workplane_uuid())
                        return Constraint::Type::POINT_ON_BEZIER;
                }
            }
        }
    }
    return {};
}

std::string ToolHelperConstrain::get_constrain_tip(const std::string &what)
{
    const auto ct = get_constraint_type();
    if (!ct)
        return "";
    switch (*ct) {
    case Constraint::Type::POINTS_COINCIDENT:
        return "constrain " + what + " on point";
    case Constraint::Type::POINT_ON_LINE:
        return "constrain " + what + " on line";
    case Constraint::Type::POINT_ON_BEZIER:
        return "constrain " + what + " on bezier";
    case Constraint::Type::MIDPOINT:
        return "constrain " + what + " on midpoint";
    case Constraint::Type::POINT_ON_CIRCLE:
        return "constrain " + what + " on circle";
    default:
        return "";
    }
}


void ToolHelperConstrain::set_constrain_tip(const std::string &what)
{
    m_intf.tool_bar_set_tool_tip(get_constrain_tip(what));
}


void ToolHelperConstrain::update_constraint_icons(std::vector<ConstraintType> &constraint_icons)
{
    if (auto ct = get_constraint_type())
        constraint_icons.push_back(*ct);
}

} // namespace dune3d
