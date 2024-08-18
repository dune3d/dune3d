#include "tool_constrain_point_plane_distance.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_plane_distance.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "util/selection_util.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointPlaneDistance::can_begin()
{
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);
    if (!lps)
        return false;

    if (m_tool_id == ToolID::CONSTRAIN_POINT_PLANE_DISTANCE) {
        auto constraints = get_doc().find_constraints(lps->get_enps());
        for (auto constraint : constraints) {
            if (constraint->of_type(Constraint::Type::POINT_PLANE_DISTANCE))
                return false;
        }

        return true;
    }

    return true;
}

ToolResponse ToolConstrainPointPlaneDistance::begin(const ToolArgs &args)
{
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);

    auto &constraint = add_constraint<ConstraintPointPlaneDistance>();
    constraint.m_line1 = lps->lines.at(0);
    constraint.m_line2 = lps->lines.at(1);
    constraint.m_point = lps->point;
    constraint.m_distance = constraint.measure_distance(get_doc());
    constraint.m_measurement = m_tool_id == ToolID::MEASURE_POINT_PLANE_DISTANCE;


    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointPlaneDistance::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
