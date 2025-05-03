#include "tool_constrain_same_orientation.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_same_orientation.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

std::optional<std::pair<UUID, UUID>> two_wrkpl_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    using T = EntityType;
    if (en1.of_type(T::WORKPLANE, T::STEP, T::CIRCLE_3D, T::ARC_3D)
        && en2.of_type(T::WORKPLANE, T::STEP, T::CIRCLE_3D, T::ARC_3D)
        && !(en1.of_type(T::CIRCLE_3D, T::ARC_3D) && en2.of_type(T::CIRCLE_3D, T::ARC_3D)))
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

ToolBase::CanBegin ToolConstrainSameOrientation::can_begin()
{
    if (!can_create_constraint())
        return false;

    auto tw = two_wrkpl_from_selection(get_doc(), m_selection);
    if (!tw.has_value())
        return false;

    const std::set<EntityAndPoint> enps = {{tw->first, 0}, {tw->second, 0}};
    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type(enps, Constraint::Type::SAME_ORIENTATION);
}

ToolResponse ToolConstrainSameOrientation::begin(const ToolArgs &args)
{
    auto tp = two_wrkpl_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintSameOrientation>();
    constraint.m_entity1 = tp->first;
    constraint.m_entity2 = tp->second;

    return commit();
}

} // namespace dune3d
