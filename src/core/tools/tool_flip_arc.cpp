#include "tool_flip_arc.hpp"
#include "document/document.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/constraint/constraint.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolFlipArc::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::ARC_2D);
    if (!enp)
        return false;
    return enp->point == 0;
}

ToolResponse ToolFlipArc::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::ARC_2D);
    if (!enp.has_value())
        return ToolResponse::end();

    auto &arc = get_entity<EntityArc2D>(enp->entity);
    std::swap(arc.m_from, arc.m_to);

    for (auto &[uu, constraint] : get_doc().m_constraints) {
        constraint->replace_point({arc.m_uuid, 1}, {arc.m_uuid, 11});
        constraint->replace_point({arc.m_uuid, 2}, {arc.m_uuid, 1});
        constraint->replace_point({arc.m_uuid, 11}, {arc.m_uuid, 2});
    }

    return ToolResponse::commit();
}


ToolResponse ToolFlipArc::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
