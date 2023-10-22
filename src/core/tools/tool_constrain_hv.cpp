#include "tool_constrain_hv.hpp"
#include "document/document.hpp"
#include "document/entity_line3d.hpp"
#include "document/entity_line2d.hpp"
#include "document/constraint_hv.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {


bool ToolConstrainHV::can_begin()
{
    if (!m_core.get_current_workplane())
        return false;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    return tp.has_value();
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
    constraint->m_entity1 = {tp->entity1, tp->point1};
    constraint->m_entity2 = {tp->entity2, tp->point2};
    constraint->m_wrkpl = m_core.get_current_workplane();

    return ToolResponse::commit();
}


ToolResponse ToolConstrainHV::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
