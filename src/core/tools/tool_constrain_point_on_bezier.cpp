#include "tool_constrain_point_on_bezier.hpp"
#include "document/constraint/constraint_line_tangent_on_bezier.hpp"
#include "document/constraint/constraint_line_perpendicular_on_bezier.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointOnBezier::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return false;

    if (any_of(m_tool_id, ToolID::CONSTRAIN_LINE_TANGENT_ON_BEZIER, ToolID::CONSTRAIN_LINE_PERPENDICULAR_ON_BEZIER)) {
        if (!get_doc().get_entity(tp->point.entity).of_type(EntityType::LINE_2D))
            return false;
    }

    const auto enps = tp->get_enps();
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type(enps, Constraint::Type::POINT_ON_BEZIER, ConstraintType::LINE_TANGENT_ON_BEZIER,
                                   ConstraintType::LINE_PERDENDICULAR_ON_BEZIER);
}

ToolResponse ToolConstrainPointOnBezier::begin(const ToolArgs &args)
{
    auto tp = bezier_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end(); 

    ConstraintPointOnBezier *constraint;
    if (m_tool_id == ToolID::CONSTRAIN_LINE_TANGENT_ON_BEZIER)
        constraint = &add_constraint<ConstraintLineTangentOnBezier>();
    else if (m_tool_id == ToolID::CONSTRAIN_LINE_PERPENDICULAR_ON_BEZIER)
        constraint = &add_constraint<ConstraintLinePerpendicularOnBezier>();
    else
        constraint = &add_constraint<ConstraintPointOnBezier>();

    constraint->m_line = tp->line;
    constraint->m_point = tp->point;
    constraint->m_wrkpl = get_workplane_uuid();
    constraint->modify_to_satisfy(get_doc());

    return commit();
}

} // namespace dune3d
