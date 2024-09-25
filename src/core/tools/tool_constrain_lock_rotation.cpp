#include "tool_constrain_lock_rotation.hpp"
#include "document/constraint/constraint_lock_rotation.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_normal.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainLockRotation::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection);
    if (!enp)
        return false;
    auto &en = get_entity(enp->entity);
    if (!en.can_move(get_doc()))
        return false;
    if (!dynamic_cast<IEntityNormal *>(&en))
        return false;

    if (!any_entity_from_current_group(en))
        return false;

    const auto constraint_types = en.get_constraint_types(get_doc());
    if (constraint_types.contains(Constraint::Type::LOCK_ROTATION))
        return false;
    return true;
}

ToolResponse ToolConstrainLockRotation::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(m_core.get_current_document(), m_selection);
    if (!enp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintLockRotation>();
    constraint.m_entity = enp->entity;

    return commit();
}
} // namespace dune3d
