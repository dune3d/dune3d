#include "tool_constrain_coincident.hpp"
#include "document/document.hpp"
#include "document/entity_line3d.hpp"
#include "document/entity_line2d.hpp"
#include "document/ientity_in_workplane.hpp"
#include "document/constraint_points_coincident.hpp"
#include "document/constraint_point_on_line.hpp"
#include "document/constraint_point_on_circle.hpp"
#include "util/selection_util.hpp"
#include "editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

bool ToolConstrainCoincident::is_point_on_point()
{
    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    if (tp->entity1 == tp->entity2) {
        return get_entity<Entity>(tp->entity1).get_type() == Entity::Type::ARC_2D;
    }
    return tp->entity1 != tp->entity2;
}

bool ToolConstrainCoincident::is_point_on_line()
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    return true;
}

bool ToolConstrainCoincident::is_point_on_circle()
{
    auto tp = circle_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    return true;
}

bool ToolConstrainCoincident::can_begin()
{
    return is_point_on_line() || is_point_on_point() || is_point_on_circle();
}

ToolResponse ToolConstrainCoincident::begin(const ToolArgs &args)
{
    if (is_point_on_point()) {
        auto tp = two_points_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointsCoincident>();
        constraint.m_entity1 = {tp->entity1, tp->point1};
        constraint.m_entity2 = {tp->entity2, tp->point2};
        constraint.m_wrkpl = m_core.get_current_workplane();
    }
    else if (is_point_on_line()) {
        auto tp = line_and_point_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointOnLine>();
        constraint.m_line = tp->line;
        constraint.m_point = {tp->point, tp->point_point};
        constraint.m_wrkpl = m_core.get_current_workplane();
    }
    else if (is_point_on_circle()) {
        auto tp = circle_and_point_from_selection(get_doc(), m_selection);

        if (!tp.has_value())
            return ToolResponse::end();

        auto &constraint = add_constraint<ConstraintPointOnCircle>();
        constraint.m_circle = tp->line;
        constraint.m_point = {tp->point, tp->point_point};
        constraint.m_wrkpl = m_core.get_current_workplane();
    }
    else {
        return ToolResponse::end();
    }

    return ToolResponse::commit();
}


ToolResponse ToolConstrainCoincident::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
