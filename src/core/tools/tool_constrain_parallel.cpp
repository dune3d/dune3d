#include "tool_constrain_parallel.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_parallel.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

static std::optional<std::pair<UUID, UUID>> two_entities_from_selection(const Document &doc,
                                                                        const std::set<SelectableRef> &sel_all)
{
    const auto sel = entities_from_selection(sel_all);

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
    const auto t1 = en1.get_type();
    const auto t2 = en2.get_type();
    if ((t1 == Entity::Type::LINE_3D && sr1.point == 0 && t2 == Entity::Type::WORKPLANE)
        || (t1 == Entity::Type::WORKPLANE && t2 == Entity::Type::LINE_3D && sr2.point == 0)
        || ((t1 == Entity::Type::LINE_2D || t1 == Entity::Type::LINE_3D)
            && (t2 == Entity::Type::LINE_2D || t2 == Entity::Type::LINE_3D) && (sr1.point == 0) && (sr2.point == 0)))
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

ToolBase::CanBegin ToolConstrainParallel::can_begin()
{
    auto tp = two_entities_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return false;

    std::set<EntityAndPoint> enps = {{tp->first, 0}, {tp->second, 0}};

    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::PARALLEL, Constraint::Type::LINES_PERPENDICULAR,
                                                Constraint::Type::LINES_ANGLE);
}

ToolResponse ToolConstrainParallel::begin(const ToolArgs &args)
{
    auto tp = two_entities_from_selection(get_doc(), m_selection);
    if (!tp)
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintParallel>();
    constraint.m_entity1 = tp->first;
    constraint.m_entity2 = tp->second;
    constraint.m_wrkpl = get_workplane_uuid();

    return commit();
}

} // namespace dune3d
