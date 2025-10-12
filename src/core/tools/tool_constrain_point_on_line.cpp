#include "tool_constrain_point_on_line.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
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

bool ToolConstrainPointOnLine::is_force_unset_workplane()
{
    return any_of(m_tool_id, ToolID::CONSTRAIN_POINT_ON_LINE_3D, ToolID::CONSTRAIN_MIDPOINT_3D);
}

bool ToolConstrainPointOnLine::constraint_is_in_workplane()
{
    return get_workplane_uuid() != UUID{};
}

ToolID ToolConstrainPointOnLine::get_force_unset_workplane_tool()
{
    auto wrkpl = get_workplane_uuid();
    if (!wrkpl)
        return ToolID::NONE;

    auto lp = line_and_point_from_selection(get_doc(), m_selection);
    if (!lp)
        return ToolID::NONE;
    ;
    if (all_entities_in_current_workplane(lp->get_enps()))
        return ToolID::NONE;

    switch (m_tool_id) {
    case ToolID::CONSTRAIN_MIDPOINT:
        return ToolID::CONSTRAIN_MIDPOINT_3D;

    case ToolID::CONSTRAIN_POINT_ON_LINE:
        return ToolID::CONSTRAIN_POINT_ON_LINE_3D;

    default:
        return ToolID::NONE;
    }
}

ToolResponse ToolConstrainPointOnLine::begin(const ToolArgs &args)
{
    auto tp = line_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    ConstraintPointOnLineBase *constraint = nullptr;

    switch (m_tool_id) {
    case ToolID::CONSTRAIN_MIDPOINT:
    case ToolID::CONSTRAIN_MIDPOINT_3D: {
        auto &c = add_constraint<ConstraintMidpoint>();
        constraint = &c;
    } break;

    case ToolID::CONSTRAIN_POINT_ON_LINE:
    case ToolID::CONSTRAIN_POINT_ON_LINE_3D: {
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
