#include "tool_constrain_point_plane_distance.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_plane_distance.hpp"
#include "tool_common_constrain_impl.hpp"
#include "util/selection_util.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointPlaneDistance::can_begin()
{
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);
    if (!lps)
        return false;

    const auto enps = lps->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type(enps, Constraint::Type::POINT_PLANE_DISTANCE);
}

ToolResponse ToolConstrainPointPlaneDistance::begin(const ToolArgs &args)
{
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);

    if (!lps.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointPlaneDistance>();
    constraint.m_line1 = lps->lines.at(0);
    constraint.m_line2 = lps->lines.at(1);
    constraint.m_point = lps->point;
    constraint.m_distance = constraint.measure_distance(get_doc());
    constraint.m_measurement = m_tool_id == ToolID::MEASURE_POINT_PLANE_DISTANCE;

    return commit();
}

} // namespace dune3d
