#include "tool_constrain_point_on_point.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

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

    const auto enps = tp->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(
            enps, Constraint::Type::POINT_DISTANCE, Constraint::Type::POINTS_COINCIDENT,
            Constraint::Type::POINT_DISTANCE_HORIZONTAL, Constraint::Type::POINT_DISTANCE_VERTICAL);
}

ToolID ToolConstrainPointOnPoint::get_force_unset_workplane_tool()
{
    auto wrkpl = get_workplane_uuid();
    if (!wrkpl)
        return ToolID::NONE;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return ToolID::NONE;

    if (all_entities_in_current_workplane(tp->get_enps()))
        return ToolID::NONE;

    return ToolID::CONSTRAIN_POINT_ON_POINT_3D;
}

bool ToolConstrainPointOnPoint::constraint_is_in_workplane()
{
    return get_workplane_uuid() != UUID{};
}

bool ToolConstrainPointOnPoint::is_force_unset_workplane()
{
    return m_tool_id == ToolID::CONSTRAIN_POINT_ON_POINT_3D;
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

    return commit();
}

} // namespace dune3d
