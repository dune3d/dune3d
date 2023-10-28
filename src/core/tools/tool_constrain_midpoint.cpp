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
    return line_and_point_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainMidpoint::begin(const ToolArgs &args)
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();


    auto &constraint = add_constraint<ConstraintMidpoint>();
    constraint.m_line = tp->line;
    constraint.m_point = {tp->point, tp->point_point};
    constraint.m_wrkpl = m_core.get_current_workplane();

    return ToolResponse::commit();
}

ToolResponse ToolConstrainMidpoint::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
