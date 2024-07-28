#include "tool_create_cluster.hpp"
#include "document/document.hpp"
#include "document/entity/entity_cluster.hpp"
#include "document/entity/ientity_in_workplane_set.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "document/group/group_reference.hpp"
#include "tool_common_impl.hpp"
#include "editor/editor_interface.hpp"
#include <iostream>
#include <algorithm>

namespace dune3d {

ToolBase::CanBegin ToolCreateCluster::can_begin()
{
    if (!get_workplane_uuid())
        return false;
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            if (entity.can_delete(get_doc())
                && entity.of_type(Entity::Type::LINE_2D, Entity::Type::ARC_2D, Entity::Type::CIRCLE_2D,
                                  Entity::Type::BEZIER_2D))
                return true;
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT) {
            return true;
        }
    }
    return false;
}

ToolResponse ToolCreateCluster::begin(const ToolArgs &args)
{
    auto &doc = get_doc();

    ItemsToDelete items_to_delete;

    auto &en_cluster = add_entity<EntityCluster>();
    en_cluster.m_wrkpl = get_workplane_uuid();

    std::set<const Constraint *> constraints;
    auto cloned_wrkpl_uu = get_doc().get_reference_group().get_workplane_xy_uuid();

    auto content = ClusterContent::create();

    for (auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &en = doc.get_entity(sr.item);
            if (!en.can_delete(doc) || !EntityCluster::is_supported_entity(en))
                continue;

            items_to_delete.entities.insert(sr.item);
            {
                auto cs = en.get_constraints(get_doc());
                constraints.insert(cs.begin(), cs.end());
            }
            {
                auto en_cloned = en.clone();
                dynamic_cast<IEntityInWorkplaneSet &>(*en_cloned).set_workplane(cloned_wrkpl_uu);
                content->m_entities.emplace(en.m_uuid, std::move(en_cloned));
            }
        }
    }
    for (auto constraint : constraints) {
        content->m_constraints.emplace(constraint->m_uuid, constraint->clone());
    }
    en_cluster.m_content = content;
    ItemsToDelete selected_items = items_to_delete;

    auto extra_items = doc.get_additional_items_to_delete(items_to_delete);
    items_to_delete.append(extra_items);

    m_intf.show_delete_items_popup(selected_items, items_to_delete);

    doc.delete_items(items_to_delete);

    return ToolResponse::commit();
}


ToolResponse ToolCreateCluster::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
