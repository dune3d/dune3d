#include "tool_constrain_point_on_line.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "util/selection_util.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnLine::can_begin()
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;
    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_ON_LINE, Constraint::Type::MIDPOINT,
                                Constraint::Type::POINT_LINE_DISTANCE))
            return false;
    }
    return true;
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

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointOnLine::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
