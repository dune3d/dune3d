#include "tool_toggle_construction.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"
#include <algorithm>


namespace dune3d {

ToolBase::CanBegin ToolToggleConstruction::can_begin()
{
    auto entities = get_entities();
    if (entities.size() == 0)
        return false;
    const bool has_normal = std::ranges::any_of(entities, [](auto x) { return !x->m_construction; });
    const bool has_construction = std::ranges::any_of(entities, [](auto x) { return x->m_construction; });
    switch (m_tool_id) {
    case ToolID::TOGGLE_CONSTRUCTION:
        if (has_normal && has_construction)
            return CanBegin::YES;
        else
            return CanBegin::YES_NO_MENU;

    case ToolID::SET_CONSTRUCTION:
        return has_normal;

    case ToolID::UNSET_CONSTRUCTION:
        return has_construction;

    default:
        return false;
    }
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

    const Group *first_group = nullptr;

    for (auto en : entities) {
        get_doc().accumulate_first_group(first_group, en->m_group);
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
    if (first_group)
        get_doc().set_group_generate_pending(first_group->m_uuid);

    return ToolResponse::commit();
}


ToolResponse ToolToggleConstruction::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
