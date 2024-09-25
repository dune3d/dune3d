#include "tool_constrain_point_on_circle.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnCircle::can_begin()
{
    auto tp = circle_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;

    const auto enps = tp->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type(enps, Constraint::Type::POINT_ON_CIRCLE);
}

ToolResponse ToolConstrainPointOnCircle::begin(const ToolArgs &args)
{
    auto tp = circle_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointOnCircle>();
    constraint.m_circle = tp->line;
    constraint.m_point = tp->point;

    return commit();
}

} // namespace dune3d
