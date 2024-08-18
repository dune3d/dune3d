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

ToolBase::CanBegin ToolConstrainDistance::can_begin()
{
    if (any_of(m_tool_id, ToolID::CONSTRAIN_DISTANCE_HORIZONTAL, ToolID::CONSTRAIN_DISTANCE_VERTICAL,
               ToolID::MEASURE_DISTANCE_HORIZONTAL, ToolID::MEASURE_DISTANCE_VERTICAL)
        && !get_workplane_uuid())
        return false;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;

    if (any_of(m_tool_id, ToolID::MEASURE_DISTANCE, ToolID::MEASURE_DISTANCE_HORIZONTAL,
               ToolID::MEASURE_DISTANCE_VERTICAL))
        return true;

    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        switch (m_tool_id) {
        case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
            if (constraint->of_type(Constraint::Type::POINT_DISTANCE_HORIZONTAL, Constraint::Type::VERTICAL,
                                    Constraint::Type::SYMMETRIC_VERTICAL))
                return false;
            break;

        case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
            if (constraint->of_type(Constraint::Type::POINT_DISTANCE_VERTICAL, Constraint::Type::HORIZONTAL,
                                    Constraint::Type::SYMMETRIC_HORIZONTAL))
                return false;
            break;

        case ToolID::CONSTRAIN_DISTANCE:
            if (constraint->of_type(Constraint::Type::POINT_DISTANCE, Constraint::Type::HORIZONTAL,
                                    Constraint::Type::VERTICAL, Constraint::Type::SYMMETRIC_HORIZONTAL,
                                    Constraint::Type::SYMMETRIC_VERTICAL))
                return false;
            break;

        default:
            return false;
        }
    }
    return true;


    return false;
}

ToolResponse ToolConstrainDistance::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);

    if (!tp)
        return ToolResponse::end();

    ConstraintPointDistanceBase *constraint = nullptr;
    switch (m_tool_id) {
    case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
    case ToolID::MEASURE_DISTANCE_HORIZONTAL:
        constraint = &add_constraint<ConstraintPointDistanceHorizontal>();
        break;

    case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
    case ToolID::MEASURE_DISTANCE_VERTICAL:
        constraint = &add_constraint<ConstraintPointDistanceVertical>();
        break;

    default:
        constraint = &add_constraint<ConstraintPointDistance>();
    }

    if (any_of(m_tool_id, ToolID::MEASURE_DISTANCE, ToolID::MEASURE_DISTANCE_HORIZONTAL,
               ToolID::MEASURE_DISTANCE_VERTICAL))
        constraint->m_measurement = true;


    constraint->m_entity1 = tp->point1;
    constraint->m_entity2 = tp->point2;
    constraint->m_wrkpl = get_workplane_uuid();
    auto dist = constraint->measure_distance(get_doc());
    if (dist < 0)
        constraint->flip();
    constraint->m_distance = std::abs(dist);


    reset_selection_after_constrain();
    return ToolResponse::commit();
}


ToolResponse ToolConstrainDistance::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
