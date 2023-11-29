#include "tool_constrain_diameter_radius.hpp"
#include "document/document.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/constraint/constraint_diameter_radius.hpp"
#include "util/selection_util.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

bool ToolConstrainDiameterRadius::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto uu = entity_from_selection(get_doc(), m_selection);
    if (!uu)
        return false;
    auto type = get_doc().get_entity(*uu).get_type();
    return type == Entity::Type::ARC_2D || type == Entity::Type::CIRCLE_2D;
}

ToolResponse ToolConstrainDiameterRadius::begin(const ToolArgs &args)
{
    auto uu = entity_from_selection(m_core.get_current_document(), m_selection);
    if (!uu.has_value())
        return ToolResponse::end();

    ConstraintDiameterRadius *constraint = nullptr;
    switch (m_tool_id) {
    case ToolID::CONSTRAIN_RADIUS:
        constraint = &add_constraint<ConstraintRadius>();
        break;

    case ToolID::CONSTRAIN_DIAMETER:
        constraint = &add_constraint<ConstraintDiameter>();
        break;

    default:
        return ToolResponse::end();
    }

    constraint->m_entity = *uu;
    constraint->measure(get_doc());

    return ToolResponse::commit();
}


ToolResponse ToolConstrainDiameterRadius::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
