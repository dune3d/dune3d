#include "tool_constrain_midpoint.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {
bool ToolConstrainMidpoint::can_begin()
{
    auto lp = line_and_point_from_selection(get_doc(), m_selection);
    if (!lp)
        return false;

    auto constraints = get_doc().find_constraints(lp->get_enps());
    for (auto constraint : constraints) {
        if (constraint->of_type(Constraint::Type::POINT_LINE_DISTANCE, Constraint::Type::POINT_ON_LINE,
                                Constraint::Type::MIDPOINT))
            return false;
    }

    return true;
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

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainMidpoint::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
