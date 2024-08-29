#include "tool_constrain_equal_length.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_equal_length.hpp"
#include <list>
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {
static std::list<UUID> lines_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    return entities_from_selection(doc, sel, {Entity::Type::LINE_2D, Entity::Type::LINE_3D});
}

ToolBase::CanBegin ToolConstrainEqualLength::can_begin()
{
    auto lines = lines_from_selection(get_doc(), m_selection);

    if (lines.size() < 2)
        return false;

    std::set<EntityAndPoint> enps;
    for (const auto &uu : lines) {
        enps.emplace(uu, 0);
    }

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::EQUAL_LENGTH);
}

ToolResponse ToolConstrainEqualLength::begin(const ToolArgs &args)
{
    auto lines = lines_from_selection(get_doc(), m_selection);

    auto it = lines.begin();
    auto first = it++;

    for (; it != lines.end(); it++) {
        auto &constraint = add_constraint<ConstraintEqualLength>();
        constraint.m_entity1 = *first;
        constraint.m_entity2 = *it;
        constraint.m_wrkpl = get_workplane_uuid();
    }

    return commit();
}
} // namespace dune3d
