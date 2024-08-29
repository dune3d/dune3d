#include "tool_constrain_point_on_bezier.hpp"
#include "document/constraint/constraint_point_on_bezier.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnBezier::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;

    return !has_constraint_of_type(tp->get_enps(), Constraint::Type::POINT_ON_BEZIER);
}

ToolResponse ToolConstrainPointOnBezier::begin(const ToolArgs &args)
{
    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointOnBezier>();
    constraint.m_line = tp->line;
    constraint.m_point = tp->point;
    constraint.m_wrkpl = get_workplane_uuid();
    constraint.modify_to_satisfy(get_doc());

    return commit();
}

} // namespace dune3d
