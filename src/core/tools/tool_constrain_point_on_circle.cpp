#include "tool_constrain_point_on_circle.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_on_circle.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnCircle::can_begin()
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

ToolResponse ToolConstrainPointOnCircle::begin(const ToolArgs &args)
{
    auto tp = circle_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointOnCircle>();
    constraint.m_circle = tp->line;
    constraint.m_point = tp->point;

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointOnCircle::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
