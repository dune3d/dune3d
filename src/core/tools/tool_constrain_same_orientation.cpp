#include "tool_constrain_same_orientation.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_same_orientation.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

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
    if (en1.get_type() == Entity::Type::WORKPLANE && en2.get_type() == Entity::Type::WORKPLANE)
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

bool ToolConstrainSameOrientation::can_begin()
{
    return two_wrkpl_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainSameOrientation::begin(const ToolArgs &args)
{
    auto tp = two_wrkpl_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();


    auto &constraint = add_constraint<ConstraintSameOrientation>();
    constraint.m_entity1 = tp->first;
    constraint.m_entity2 = tp->second;

    return ToolResponse::commit();
}

ToolResponse ToolConstrainSameOrientation::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
