#include "tool_constrain_coincident.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "document/constraint/constraint_point_on_bezier.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

bool ToolConstrainCoincident::is_point_on_point()
{
    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    if (tp->point1.entity == tp->point2.entity
        && ((tp->point1.point == 2 && tp->point2.point == 1) || (tp->point1.point == 1 && tp->point2.point == 2))) {
        return get_entity<Entity>(tp->point1.entity).of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D);
    }
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_DISTANCE, Constraint::Type::POINTS_COINCIDENT,
                                Constraint::Type::POINT_DISTANCE_HORIZONTAL, Constraint::Type::POINT_DISTANCE_VERTICAL))
            return false;
    }
    return tp->point1.entity != tp->point2.entity;
}

bool ToolConstrainCoincident::is_point_on_line()
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_ON_LINE, Constraint::Type::MIDPOINT,
                                Constraint::Type::POINT_LINE_DISTANCE))
            return false;
    }
    return true;
}

bool ToolConstrainCoincident::is_point_on_bezier()
{
    if (!get_workplane_uuid())
        return false;
    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_ON_BEZIER))
            return false;
    }
    return true;
}

bool ToolConstrainCoincident::is_point_on_circle()
{
    auto tp = circle_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_ON_CIRCLE))
            return false;
    }
    return true;
}

ToolBase::CanBegin ToolConstrainCoincident::can_begin()
{
    return is_point_on_line() || is_point_on_point() || is_point_on_circle() || is_point_on_bezier();
}

ToolResponse ToolConstrainCoincident::begin(const ToolArgs &args)
{
    if (is_point_on_point()) {
        auto tp = two_points_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointsCoincident>();
        constraint.m_entity1 = tp->point1;
        constraint.m_entity2 = tp->point2;
        constraint.m_wrkpl = get_workplane_uuid();
    }
    else if (is_point_on_line()) {
        auto tp = line_and_point_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointOnLine>();
        constraint.m_line = tp->line;
        constraint.m_point = tp->point;
        constraint.m_wrkpl = get_workplane_uuid();
        constraint.m_modify_to_satisfy = true;
    }
    else if (is_point_on_circle()) {
        auto tp = circle_and_point_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointOnCircle>();
        constraint.m_circle = tp->line;
        constraint.m_point = tp->point;
    }
    else if (is_point_on_bezier()) {
        auto tp = bezier_and_point_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointOnBezier>();
        constraint.m_line = tp->line;
        constraint.m_point = tp->point;
        constraint.m_wrkpl = get_workplane_uuid();
        constraint.modify_to_satisfy(get_doc());
    }
    else {
        return ToolResponse::end();
    }

    reset_selection_after_constrain();
    return ToolResponse::commit();
}


ToolResponse ToolConstrainCoincident::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
