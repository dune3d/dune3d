#include "tool_constrain_hv.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/constraint/constraint_hv.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {


bool ToolConstrainHV::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;

    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (m_tool_id == ToolID::CONSTRAIN_HORIZONTAL) {
            if (constraint->of_type(Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL,
                                    Constraint::Type::POINT_DISTANCE_VERTICAL))
                return false;
        }
        else {
            if (constraint->of_type(Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL,
                                    Constraint::Type::POINT_DISTANCE_HORIZONTAL))
                return false;
        }
    }

    return true;
}

ToolResponse ToolConstrainHV::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    ConstraintHV *constraint = nullptr;
    if (m_tool_id == ToolID::CONSTRAIN_HORIZONTAL)
        constraint = &add_constraint<ConstraintHorizontal>();
    else
        constraint = &add_constraint<ConstraintVertical>();
    constraint->m_entity1 = tp->point1;
    constraint->m_entity2 = tp->point2;
    constraint->m_wrkpl = get_workplane_uuid();

    reset_selection_after_constrain();
    return ToolResponse::commit();
}


ToolResponse ToolConstrainHV::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
