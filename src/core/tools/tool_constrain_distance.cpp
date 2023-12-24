#include "tool_constrain_distance.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/constraint/constraint_point_distance.hpp"
#include "document/constraint/constraint_point_distance_hv.hpp"
#include "document/constraint/constraint_point_line_distance.hpp"
#include "document/constraint/constraint_point_plane_distance.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

bool ToolConstrainDistance::can_begin()
{
    if ((m_tool_id == ToolID::CONSTRAIN_DISTANCE_HORIZONTAL || m_tool_id == ToolID::CONSTRAIN_DISTANCE_VERTICAL)
        && !get_workplane_uuid())
        return false;

    auto lp = line_and_point_from_selection(get_doc(), m_selection, LineAndPoint::AllowSameEntity::NO);
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);
    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp && !lp && !lps)
        return false;
    if (tp) {
        auto constraints = get_doc().find_constraints(tp->get_enps());
        for (auto constraint : constraints) {
            switch (m_tool_id) {
            case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
                if (constraint->of_type(Constraint::Type::POINT_DISTANCE_HORIZONTAL, Constraint::Type::VERTICAL))
                    return false;
                break;

            case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
                if (constraint->of_type(Constraint::Type::POINT_DISTANCE_VERTICAL, Constraint::Type::HORIZONTAL))
                    return false;
                break;

            case ToolID::CONSTRAIN_DISTANCE:
                if (constraint->of_type(Constraint::Type::POINT_DISTANCE, Constraint::Type::HORIZONTAL,
                                        Constraint::Type::VERTICAL))
                    return false;
                break;
            default:
                return false;
            }
        }
        return true;
    }
    else if (lp && m_tool_id == ToolID::CONSTRAIN_DISTANCE) {
        auto constraints = get_doc().find_constraints(lp->get_enps());
        for (auto constraint : constraints) {
            if (constraint->of_type(Constraint::Type::POINT_LINE_DISTANCE, Constraint::Type::POINT_ON_LINE,
                                    Constraint::Type::MIDPOINT))
                return false;
        }

        return true;
    }
    else if (lps && m_tool_id == ToolID::CONSTRAIN_DISTANCE) {
        auto constraints = get_doc().find_constraints(lp->get_enps());
        for (auto constraint : constraints) {
            if (constraint->of_type(Constraint::Type::POINT_PLANE_DISTANCE))
                return false;
        }

        return true;
    }
    return false;
}

ToolResponse ToolConstrainDistance::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);
    auto lp = line_and_point_from_selection(get_doc(), m_selection, LineAndPoint::AllowSameEntity::NO);
    auto lps = lines_and_point_from_selection(get_doc(), m_selection);

    if (!tp && !lp && !lps)
        return ToolResponse::end();

    if (tp) {
        ConstraintPointDistanceBase *constraint = nullptr;
        switch (m_tool_id) {
        case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
            constraint = &add_constraint<ConstraintPointDistanceHorizontal>();
            break;

        case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
            constraint = &add_constraint<ConstraintPointDistanceVertical>();
            break;

        default:
            constraint = &add_constraint<ConstraintPointDistance>();
        }

        constraint->m_entity1 = tp->point1;
        constraint->m_entity2 = tp->point2;
        constraint->m_wrkpl = get_workplane_uuid();
        auto dist = constraint->measure_distance(get_doc());
        if (dist < 0)
            constraint->flip();
        constraint->m_distance = std::abs(dist);
    }
    else if (lp) {
        auto &constraint = add_constraint<ConstraintPointLineDistance>();
        constraint.m_line = lp->line;
        constraint.m_point = lp->point;
        constraint.m_wrkpl = get_workplane_uuid();
        constraint.m_modify_to_satisfy = true;
    }
    else if (lps) {
        auto &constraint = add_constraint<ConstraintPointPlaneDistance>();
        constraint.m_line1 = lps->lines.at(0);
        constraint.m_line2 = lps->lines.at(1);
        constraint.m_point = lps->point;
        constraint.m_distance = constraint.measure_distance(get_doc());
    }

    reset_selection_after_constrain();
    return ToolResponse::commit();
}


ToolResponse ToolConstrainDistance::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
