#include "tool_constrain_equal_radius.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_equal_radius.hpp"
#include <list>
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {
static std::list<UUID> circles_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    return entities_from_selection(doc, sel, {Entity::Type::CIRCLE_2D, Entity::Type::ARC_2D});
}

ToolBase::CanBegin ToolConstrainEqualRadius::can_begin()
{
    auto circles = circles_from_selection(get_doc(), m_selection);

    if (circles.size() < 2)
        return false;

    std::set<EntityAndPoint> enps;
    for (const auto &uu : circles) {
        enps.emplace(uu, 0);
    }

    return !has_constraint_of_type(enps, Constraint::Type::EQUAL_RADIUS);
}

ToolResponse ToolConstrainEqualRadius::begin(const ToolArgs &args)
{
    auto circles = circles_from_selection(get_doc(), m_selection);

    auto it = circles.begin();
    auto first = it++;

    for (; it != circles.end(); it++) {
        auto &constraint = add_constraint<ConstraintEqualRadius>();
        constraint.m_entity1 = *first;
        constraint.m_entity2 = *it;
    }

    return commit();
}
} // namespace dune3d
