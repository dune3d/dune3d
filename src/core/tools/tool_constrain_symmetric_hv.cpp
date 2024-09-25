#include "tool_constrain_symmetric_hv.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint_symmetric_hv.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {


ToolBase::CanBegin ToolConstrainSymmetricHV::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;

    const auto enps = tp->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    if (m_tool_id == ToolID::CONSTRAIN_SYMMETRIC_HORIZONTAL)
        return !has_constraint_of_type_in_workplane(
                enps, Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL, Constraint::Type::SYMMETRIC_HORIZONTAL,
                Constraint::Type::SYMMETRIC_VERTICAL, Constraint::Type::POINT_DISTANCE_VERTICAL);
    else
        return !has_constraint_of_type_in_workplane(
                enps, Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL, Constraint::Type::SYMMETRIC_HORIZONTAL,
                Constraint::Type::SYMMETRIC_VERTICAL, Constraint::Type::POINT_DISTANCE_HORIZONTAL);
}

ToolResponse ToolConstrainSymmetricHV::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    ConstraintSymmetricHV *constraint = nullptr;
    if (m_tool_id == ToolID::CONSTRAIN_SYMMETRIC_VERTICAL)
        constraint = &add_constraint<ConstraintSymmetricVertical>();
    else
        constraint = &add_constraint<ConstraintSymmetricHorizontal>();
    constraint->m_entity1 = tp->point1;
    constraint->m_entity2 = tp->point2;
    constraint->m_wrkpl = get_workplane_uuid();

    return commit();
}

} // namespace dune3d
