#include "tool_constrain_point_in_plane.hpp"
#include "document/constraint/constraint_point_in_plane.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointInPlane::can_begin()
{
    return lines_and_point_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainPointInPlane::begin(const ToolArgs &args)
{
    auto tp = lines_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointInPlane>();
    constraint.m_point = tp->point;
    constraint.m_line1 = tp->lines.at(0);
    constraint.m_line2 = tp->lines.at(1);

    return commit();
}

} // namespace dune3d
