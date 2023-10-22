#include "tool_constrain_equal_radius.hpp"
#include "document/document.hpp"
#include "document/entity.hpp"
#include "document/constraint_equal_radius.hpp"
#include "core/tool_id.hpp"
#include <list>
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {
static std::list<UUID> circles_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    std::list<UUID> r;
    for (const auto &sr : sel) {
        if (sr.type != SelectableRef::Type::ENTITY)
            continue;
        if (sr.point != 0)
            continue;
        auto &en = doc.get_entity(sr.item);
        if (en.get_type() == Entity::Type::CIRCLE_2D || en.get_type() == Entity::Type::ARC_2D)
            r.push_back(sr.item);
    }
    return r;
}

bool ToolConstrainEqualRadius::can_begin()
{
    return circles_from_selection(get_doc(), m_selection).size() >= 2;
}

ToolResponse ToolConstrainEqualRadius::begin(const ToolArgs &args)
{
    auto lines = circles_from_selection(get_doc(), m_selection);

    auto it = lines.begin();
    auto first = it++;

    for (; it != lines.end(); it++) {
        auto &constraint = add_constraint<ConstraintEqualRadius>();
        constraint.m_entity1 = *first;
        constraint.m_entity2 = *it;
    }


    return ToolResponse::commit();
}

ToolResponse ToolConstrainEqualRadius::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
