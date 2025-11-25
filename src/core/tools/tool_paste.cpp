#include "tool_paste.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_step.hpp"
#include "document/entity/ientity_in_workplane_set.hpp"
#include "document/entity/ientity_movable2d.hpp"
#include "document/entity/ientity_movable2d_initial_pos.hpp"
#include "document/entity/ientity_movable3d.hpp"
#include "document/entity/ientity_cluster_content_update.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "document/group/group_reference.hpp"
#include "util/cluster_content.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "editor/buffer.hpp"
#include "canvas/selection_mode.hpp"

namespace dune3d {

ToolBase::CanBegin ToolPaste::can_begin()
{
    return m_intf.get_buffer();
}

ToolResponse ToolPaste::begin(const ToolArgs &args)
{
    if (!m_intf.get_buffer())
        return ToolResponse::end();
    const auto &buffer = *m_intf.get_buffer();
    auto &doc = get_doc();
    m_selection.clear();
    std::map<UUID, UUID> entity_xlat;
    auto wrkpl = get_workplane_uuid();
    bool created_workplane = false;
    if (!wrkpl) {
        for (const auto &[uu, en] : buffer.m_entities) {
            if (en->of_type(EntityType::WORKPLANE)) {
                created_workplane = true;
                auto new_entity = en->clone();
                new_entity->m_uuid = UUID::random();
                new_entity->m_group = m_core.get_current_group();
                new_entity->m_kind = ItemKind::USER;
                auto &en_wrkpl = dynamic_cast<EntityWorkplane &>(*new_entity);
                en_wrkpl.m_origin = m_intf.get_cursor_pos();
                if (en_wrkpl.has_name())
                    en_wrkpl.m_name = en_wrkpl.m_name + " (Copy)";

                entity_xlat.emplace(uu, new_entity->m_uuid);
                m_selection.emplace(SelectableRef::Type::ENTITY, new_entity->m_uuid, new_entity->get_point_for_move());
                wrkpl = new_entity->m_uuid;
                doc.m_entities.emplace(new_entity->m_uuid, std::move(new_entity));
                break;
            }
        }
    }

    glm::dvec2 shift2 = {0, 0};
    glm::dvec3 shift3 = {0, 0, 0};
    const auto buffer_bbox = buffer.get_bbox();
    if (!created_workplane && buffer_bbox.has_value() && wrkpl) {
        const auto old_center = (buffer_bbox->first + buffer_bbox->second) / 2.;
        const auto &en_wrkpl = get_entity<EntityWorkplane>(wrkpl);
        const auto new_center = en_wrkpl.project(get_cursor_pos_for_workplane(en_wrkpl));
        shift2 = new_center - old_center;
        shift3 = en_wrkpl.transform_relative(shift2);
    }

    for (const auto &[uu, en] : buffer.m_entities) {
        if (en->of_type(EntityType::WORKPLANE))
            continue;
        auto new_entity = en->clone();
        new_entity->m_uuid = UUID::random();
        new_entity->m_group = m_core.get_current_group();
        new_entity->m_kind = ItemKind::USER;

        if (auto en_wrkpl = dynamic_cast<IEntityInWorkplaneSet *>(new_entity.get())) {
            if (!wrkpl)
                continue;
            if (en_wrkpl->get_workplane() != buffer.m_wrkpl)
                continue;
            en_wrkpl->set_workplane(wrkpl);
        }
        if (auto en_move2 = dynamic_cast<IEntityMovable2D *>(new_entity.get())) {
            en_move2->move(*en, shift2, en->get_point_for_move());
        }
        if (auto en_move2i = dynamic_cast<IEntityMovable2DIntialPos *>(new_entity.get())) {
            en_move2i->move(*en, {0, 0}, shift2, en->get_point_for_move());
        }
        if (auto en_move3 = dynamic_cast<IEntityMovable3D *>(new_entity.get())) {
            en_move3->move(*en, shift3, en->get_point_for_move());
        }
        if (auto en_cluster = dynamic_cast<IEntityClusterContentUpdate *>(new_entity.get())) {
            en_cluster->update_cluster_content_for_new_workplane(doc.get_reference_group().get_workplane_xy_uuid());
        }
        if (auto en_step = dynamic_cast<EntitySTEP *>(new_entity.get())) {
            const auto is_sketch_group = get_group().get_type() == Group::Type::SKETCH;
            if (!is_sketch_group)
                en_step->m_include_in_solid_model = false;
        }

        entity_xlat.emplace(uu, new_entity->m_uuid);
        m_selection.emplace(SelectableRef::Type::ENTITY, new_entity->m_uuid, new_entity->get_point_for_move());
        doc.m_entities.emplace(new_entity->m_uuid, std::move(new_entity));
    }
    m_intf.set_canvas_selection_mode(SelectionMode::NORMAL);


    for (const auto &[uu, co] : buffer.m_constraints) {
        auto new_co = co->clone();
        new_co->m_uuid = UUID::random();
        new_co->m_group = m_core.get_current_group();
        auto referenced = new_co->get_referenced_entities_and_points();
        bool skip = false;
        bool replaced = false;
        for (const auto &enp : referenced) {
            if (enp.entity == buffer.m_wrkpl) {
                if (!new_co->replace_point(enp, {wrkpl, enp.point})) {
                    skip = true;
                    break;
                }
            }
            else if (entity_xlat.contains(enp.entity)) {
                if (!new_co->replace_point(enp, {entity_xlat.at(enp.entity), enp.point})) {
                    skip = true;
                    break;
                }
                else {
                    replaced = true;
                }
            }
            else {
                if (!doc.m_entities.contains(enp.entity)) {
                    skip = true;
                    break;
                }
                auto &en = get_entity(enp.entity);
                auto &group = doc.get_group(en.m_group);
                if (group.get_index() > get_group().get_index()) {
                    skip = true;
                    break;
                }
            }
        }
        if (!replaced)
            skip = true;

        if (!skip)
            doc.m_constraints.emplace(new_co->m_uuid, std::move(new_co));
    }

    set_current_group_generate_pending();

    return ToolResponse::commit();
}

ToolResponse ToolPaste::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
