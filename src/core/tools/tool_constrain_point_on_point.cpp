#include "tool_constrain_point_on_point.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnPoint::can_begin()
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

ToolResponse ToolConstrainPointOnPoint::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointsCoincident>();
    constraint.m_entity1 = tp->point1;
    constraint.m_entity2 = tp->point2;
    constraint.m_wrkpl = get_workplane_uuid();

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointOnPoint::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
