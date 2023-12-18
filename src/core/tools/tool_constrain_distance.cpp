#include "tool_constrain_distance.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/constraint/constraint_point_distance.hpp"
#include "document/constraint/constraint_point_distance_hv.hpp"
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

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;
    if (tp->entity1 == tp->entity2) {
        // single entity
        auto &en = get_entity(tp->entity1);
        const auto constraint_types = en.get_constraint_types(get_doc());
        switch (m_tool_id) {
        case ToolID::CONSTRAIN_DISTANCE_HORIZONTAL:
            if (set_contains(constraint_types, Constraint::Type::POINT_DISTANCE_HORIZONTAL, Constraint::Type::VERTICAL))
                return false;
            break;

        case ToolID::CONSTRAIN_DISTANCE_VERTICAL:
            if (set_contains(constraint_types, Constraint::Type::POINT_DISTANCE_VERTICAL, Constraint::Type::HORIZONTAL))
                return false;
            break;

        case ToolID::CONSTRAIN_DISTANCE:
            if (set_contains(constraint_types, Constraint::Type::POINT_DISTANCE, Constraint::Type::HORIZONTAL,
                             Constraint::Type::VERTICAL))
                return false;
            break;
        default:
            return false;
        }
    }

    return true;
}

ToolResponse ToolConstrainDistance::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp.has_value())
        return ToolResponse::end();

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

    constraint->m_entity1 = {tp->entity1, tp->point1};
    constraint->m_entity2 = {tp->entity2, tp->point2};
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
