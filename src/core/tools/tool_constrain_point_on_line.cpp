#include "tool_constrain_point_on_line.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnLine::can_begin()
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;

    const auto enps = tp->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::POINT_ON_LINE, Constraint::Type::MIDPOINT,
                                                Constraint::Type::POINT_LINE_DISTANCE);
}

ToolResponse ToolConstrainPointOnLine::begin(const ToolArgs &args)
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    ConstraintPointOnLineBase *constraint = nullptr;

    switch (m_tool_id) {
    case ToolID::CONSTRAIN_MIDPOINT: {
        auto &c = add_constraint<ConstraintMidpoint>();
        constraint = &c;
    } break;

    case ToolID::CONSTRAIN_POINT_ON_LINE: {
        auto &c = add_constraint<ConstraintPointOnLine>();
        c.m_modify_to_satisfy = true;
        constraint = &c;
    } break;

    default:
        return ToolResponse::end();
    }
    constraint->m_line = tp->line;
    constraint->m_point = tp->point;
    constraint->m_wrkpl = get_workplane_uuid();

    return commit();
}

} // namespace dune3d
