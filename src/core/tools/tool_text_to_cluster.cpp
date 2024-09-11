#include "tool_text_to_cluster.hpp"
#include "document/document.hpp"
#include "document/entity/entity_text.hpp"
#include "document/entity/entity_cluster.hpp"
#include "util/selection_util.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolTextToCluster::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::TEXT);
    if (!enp)
        return false;
    return enp->point == 0;
}

ToolResponse ToolTextToCluster::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::TEXT);
    if (!enp)
        return ToolResponse::end();
    {
        const auto &en_text = get_entity<EntityText>(enp->entity);
        auto &en_cluster = add_entity<EntityCluster>();
        en_cluster.m_content = en_text.m_content;
        en_cluster.m_origin = en_text.m_origin;
        en_cluster.m_scale_x = en_text.m_scale;
        en_cluster.m_scale_y = en_text.m_scale;
        en_cluster.m_lock_scale_x = en_text.m_lock_scale;
        en_cluster.m_lock_scale_y = en_text.m_lock_scale;
        en_cluster.m_angle = en_text.m_angle;
        en_cluster.m_lock_angle = en_text.m_lock_angle;
        en_cluster.m_group = en_text.m_group;
        en_cluster.m_wrkpl = en_text.m_wrkpl;

        get_doc().set_group_generate_pending(en_cluster.m_group);
    }

    ItemsToDelete selected_items;
    selected_items.entities.insert(enp->entity);
    ItemsToDelete items_to_delete = selected_items;

    auto extra_items = get_doc().get_additional_items_to_delete(items_to_delete);
    items_to_delete.append(extra_items);

    m_intf.show_delete_items_popup(selected_items, items_to_delete);

    get_doc().delete_items(items_to_delete);

    return ToolResponse::commit();
}

ToolResponse ToolTextToCluster::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
