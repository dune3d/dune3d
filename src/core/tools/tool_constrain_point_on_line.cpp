#include "tool_constrain_point_on_line.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnLine::can_begin()
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;

    return !has_constraint_of_type_in_workplane(tp->get_enps(), Constraint::Type::POINT_ON_LINE,
                                                Constraint::Type::MIDPOINT, Constraint::Type::POINT_LINE_DISTANCE);
}

ToolResponse ToolConstrainPointOnLine::begin(const ToolArgs &args)
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointOnLine>();
    constraint.m_line = tp->line;
    constraint.m_point = tp->point;
    constraint.m_wrkpl = get_workplane_uuid();
    constraint.m_modify_to_satisfy = true;

    return commit();
}

} // namespace dune3d
