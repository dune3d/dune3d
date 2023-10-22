#include "tool_toggle_construction.hpp"
#include "document/document.hpp"
#include "document/entity.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"


namespace dune3d {


ToolResponse ToolToggleConstruction::begin(const ToolArgs &args)
{
    std::set<Entity *> entities;

    for (auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &en = get_entity(sr.item);
            entities.insert(&en);
        }
    }
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
