#include "tool_constrain_midpoint.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {
ToolBase::CanBegin ToolConstrainMidpoint::can_begin()
{
    auto lp = line_and_point_from_selection(get_doc(), m_selection);
    if (!lp)
        return false;

    return !has_constraint_of_type_in_workplane(lp->get_enps(), Constraint::Type::POINT_LINE_DISTANCE,
                                                Constraint::Type::POINT_ON_LINE, Constraint::Type::MIDPOINT);
}

ToolResponse ToolConstrainMidpoint::begin(const ToolArgs &args)
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintMidpoint>();
    constraint.m_line = tp->line;
    constraint.m_point = tp->point;
    constraint.m_wrkpl = get_workplane_uuid();

    return commit();
}

} // namespace dune3d
