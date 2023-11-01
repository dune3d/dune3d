#include "tool_toggle_construction.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"
#include <algorithm>


namespace dune3d {

bool ToolToggleConstruction::can_begin()
{
    auto entities = get_entities();
    if (entities.size() == 0)
        return false;
    const bool has_normal = std::ranges::any_of(entities, [](auto x) { return !x->m_construction; });
    const bool has_construction = std::ranges::any_of(entities, [](auto x) { return x->m_construction; });
    if (m_tool_id == ToolID::TOGGLE_CONSTRUCTION)
        return has_normal && has_construction;
    else if (m_tool_id == ToolID::SET_CONSTRUCTION)
        return has_normal;
    else if (m_tool_id == ToolID::UNSET_CONSTRUCTION)
        return has_construction;
    return false;
}

std::set<Entity *> ToolToggleConstruction::get_entities()
{
    std::set<Entity *> entities;
    for (auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &en = get_entity(sr.item);
            entities.insert(&en);
        }
    }
    return entities;
}

ToolResponse ToolToggleConstruction::begin(const ToolArgs &args)
{
    auto entities = get_entities();
    if (entities.size() == 0)
        return ToolResponse::end();

    for (auto en : entities) {
        switch (m_tool_id) {
        case ToolID::TOGGLE_CONSTRUCTION:
            en->m_construction = !en->m_construction;
            break;
        case ToolID::SET_CONSTRUCTION:
            en->m_construction = true;
            break;
        case ToolID::UNSET_CONSTRUCTION:
            en->m_construction = false;
            break;

        default:;
        }
    }

    return ToolResponse::commit();
}


ToolResponse ToolToggleConstruction::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
