#include "tool_constrain_point_line_distance.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint_point_line_distance.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainPointLineDistance::can_begin()
{
    auto lp = line_and_point_from_selection(get_doc(), m_selection, LineAndPoint::AllowSameEntity::NO);
    if (!lp)
        return false;
    if (m_tool_id == ToolID::CONSTRAIN_POINT_LINE_DISTANCE) {
        auto constraints = get_doc().find_constraints(lp->get_enps());
        for (auto constraint : constraints) {
            if (constraint->of_type(Constraint::Type::POINT_LINE_DISTANCE, Constraint::Type::POINT_ON_LINE,
                                    Constraint::Type::MIDPOINT))
                return false;
        }

        return true;
    }
    return true;
}

ToolResponse ToolConstrainPointLineDistance::begin(const ToolArgs &args)
{
    auto lp = line_and_point_from_selection(get_doc(), m_selection, LineAndPoint::AllowSameEntity::NO);

    if (!lp)
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointLineDistance>();
    constraint.m_line = lp->line;
    constraint.m_point = lp->point;
    constraint.m_wrkpl = get_workplane_uuid();
    constraint.m_modify_to_satisfy = true;
    constraint.m_measurement = m_tool_id == ToolID::MEASURE_POINT_LINE_DISTANCE;

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointLineDistance::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
