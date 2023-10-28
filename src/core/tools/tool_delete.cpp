#include "tool_delete.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "tool_common_impl.hpp"
#include <iostream>

namespace dune3d {

bool ToolDelete::can_begin()
{
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            if (entity.can_delete(get_doc()))
                return true;
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT) {
            return true;
        }
    }
    return false;
}

ToolResponse ToolDelete::begin(const ToolArgs &args)
{
    auto &doc = get_doc();

    Document::ItemsToDelete items_to_delete;

    for (auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &en = doc.get_entity(sr.item);
            if (!en.can_delete(doc))
                continue;
            if (auto en_step = dynamic_cast<EntitySTEP *>(&en)) {
                if (en_step->m_anchors.contains(sr.point))
                    en_step->remove_anchor(sr.point);
                else
                    items_to_delete.entities.insert(sr.item);
            }
            else {
                items_to_delete.entities.insert(sr.item);
            }
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT)
            items_to_delete.constraints.insert(sr.item);
    }

    auto extra_items = doc.get_additional_items_to_delete(items_to_delete);
    for (const auto &uu : extra_items.entities) {
        std::cout << "delete entity " << (std::string)uu << std::endl;
    }
    for (const auto &uu : extra_items.groups) {
        std::cout << "delete group " << (std::string)uu << std::endl;
    }
    for (const auto &uu : extra_items.constraints) {
        std::cout << "delete constraint " << (std::string)uu << std::endl;
    }
    items_to_delete.append(extra_items);
    doc.delete_items(items_to_delete);

    return ToolResponse::commit();
}


ToolResponse ToolDelete::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
