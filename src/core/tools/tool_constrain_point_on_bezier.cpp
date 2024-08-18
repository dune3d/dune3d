#include "tool_constrain_point_on_bezier.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_on_bezier.hpp"
#include "util/selection_util.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnBezier::can_begin()
{
    if (!get_workplane_uuid())
        return false;
    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_ON_BEZIER))
            return false;
    }
    return true;
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

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointOnBezier::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
