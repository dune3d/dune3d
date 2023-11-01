#include "tool_move.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_step.hpp"
#include "document/constraint/constraint_point_distance.hpp"
#include "document/constraint/constraint_diameter_radius.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

bool ToolMove::can_begin()
{
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            if (entity.can_move(get_doc()))
                return true;
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT) {
            auto &constr = get_doc().get_constraint(sr.item);
            if (constr.is_movable())
                return true;
        }
    }
    return false;
}

ToolResponse ToolMove::begin(const ToolArgs &args)
{
    auto &doc = get_doc();
    m_inital_pos = m_intf.get_cursor_pos();
    for (const auto &[uu, en] : doc.m_entities) {
        if (en->get_type() == Entity::Type::WORKPLANE) {
            auto &wrkpl = dynamic_cast<const EntityWorkplane &>(*en);
            m_inital_pos_wrkpl.emplace(
                    uu, wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal())));
        }
    }
    const Group *first_group = nullptr;
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            get_doc().accumulate_first_group(first_group, entity.m_group);
        }
        // we don't care about constraints since dragging them is pureley cosmetic
    }
    if (first_group)
        m_first_group = first_group->m_uuid;

    return ToolResponse();
}


ToolResponse ToolMove::update(const ToolArgs &args)
{
    auto &doc = get_doc();
    auto &last_doc = m_core.get_current_last_document();
    if (args.type == ToolEventType::MOVE) {
        const auto delta = m_intf.get_cursor_pos() - m_inital_pos;
        for (const auto &sr : m_selection) {
            if (sr.type == SelectableRef::Type::ENTITY) {
                auto entity = doc.m_entities.at(sr.item).get();
                if (!entity->can_move(doc))
                    continue;
                if (auto en = dynamic_cast<EntityLine3D *>(entity)) {
                    auto &en_last = dynamic_cast<const EntityLine3D &>(*last_doc.m_entities.at(sr.item));
                    if (sr.point == 0 || sr.point == 1) {
                        en->m_p1 = en_last.m_p1 + delta;
                    }
                    if (sr.point == 0 || sr.point == 2) {
                        en->m_p2 = en_last.m_p2 + delta;
                    }
                }
                if (auto en = dynamic_cast<EntityWorkplane *>(entity)) {
                    auto &en_last = dynamic_cast<const EntityWorkplane &>(*last_doc.m_entities.at(sr.item));
                    en->m_origin = en_last.m_origin + delta;
                }
                if (auto en = dynamic_cast<EntitySTEP *>(entity)) {
                    auto &en_last = dynamic_cast<const EntitySTEP &>(*last_doc.m_entities.at(sr.item));
                    if (sr.point == 0 || sr.point == 1)
                        en->m_origin = en_last.m_origin + delta;
                    else if (en->m_anchors_transformed.contains(sr.point)
                             && en_last.m_anchors_transformed.contains(sr.point))
                        en->m_anchors_transformed.at(sr.point) = en_last.m_anchors_transformed.at(sr.point) + delta;
                }
                if (auto en = dynamic_cast<EntityLine2D *>(entity)) {
                    auto &en_last = dynamic_cast<const EntityLine2D &>(*last_doc.m_entities.at(sr.item));
                    auto &wrkpl = dynamic_cast<const EntityWorkplane &>(*doc.m_entities.at(en->m_wrkpl));
                    const auto delta2d =
                            wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal()))
                            - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                    if (sr.point == 0 || sr.point == 1) {
                        en->m_p1 = en_last.m_p1 + delta2d;
                    }
                    if (sr.point == 0 || sr.point == 2) {
                        en->m_p2 = en_last.m_p2 + delta2d;
                    }
                }
                if (auto en = dynamic_cast<EntityArc2D *>(entity)) {
                    auto &en_last = dynamic_cast<const EntityArc2D &>(*last_doc.m_entities.at(sr.item));
                    auto &wrkpl = dynamic_cast<const EntityWorkplane &>(*doc.m_entities.at(en->m_wrkpl));
                    const auto delta2d =
                            wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal()))
                            - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                    if (sr.point == 0 || sr.point == 1) {
                        en->m_from = en_last.m_from + delta2d;
                    }
                    if (sr.point == 0 || sr.point == 2) {
                        en->m_to = en_last.m_to + delta2d;
                    }
                    if (sr.point == 0 || sr.point == 3) {
                        en->m_center = en_last.m_center + delta2d;
                    }
                }
                if (auto en = dynamic_cast<EntityCircle2D *>(entity)) {
                    auto &en_last = dynamic_cast<const EntityCircle2D &>(*last_doc.m_entities.at(sr.item));
                    auto &wrkpl = dynamic_cast<const EntityWorkplane &>(*doc.m_entities.at(en->m_wrkpl));

                    if (sr.point == 1) {
                        const auto delta2d =
                                wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal()))
                                - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                        en->m_center = en_last.m_center + delta2d;
                    }
                    else if (sr.point == 0) {
                        const auto initial_radius = glm::length(en_last.m_center - m_inital_pos_wrkpl.at(wrkpl.m_uuid));
                        const auto current_radius = glm::length(
                                en->m_center
                                - wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal())));


                        en->m_radius = en_last.m_radius + (current_radius - initial_radius);
                    }
                }
            }
            else if (sr.type == SelectableRef::Type::CONSTRAINT) {
                auto constraint = doc.m_constraints.at(sr.item).get();
                if (auto co = dynamic_cast<ConstraintPointDistanceBase *>(constraint)) {
                    auto cdelta = delta;
                    if (co->m_wrkpl) {
                        auto &wrkpl = doc.get_entity<EntityWorkplane>(co->m_wrkpl);
                        const auto delta2d =
                                wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal()))
                                - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                        cdelta = wrkpl.transform_relative(delta2d);
                    }
                    auto &co_last =
                            dynamic_cast<const ConstraintPointDistanceBase &>(*last_doc.m_constraints.at(sr.item));
                    co->m_offset = co_last.m_offset + cdelta;
                }
                if (auto co = dynamic_cast<ConstraintDiameterRadius *>(constraint)) {
                    auto &en = doc.get_entity(co->m_entity);
                    auto &en_wrkpl = dynamic_cast<const IEntityInWorkplane &>(en);
                    auto &wrkpl = doc.get_entity<EntityWorkplane>(en_wrkpl.get_workplane());
                    const auto delta2d =
                            wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal()))
                            - m_inital_pos_wrkpl.at(wrkpl.m_uuid);

                    auto &co_last = dynamic_cast<const ConstraintDiameterRadius &>(*last_doc.m_constraints.at(sr.item));
                    co->m_offset = co_last.m_offset + delta2d;
                }
            }
        }

        ICore::DraggedList dragged_entities;
        for (const auto &sr : m_selection) {
            if (sr.type == SelectableRef::Type::ENTITY)
                dragged_entities.emplace_back(sr.item, sr.point);
        }

        doc.set_group_solve_pending(m_first_group);
        m_core.solve_current(dragged_entities);

        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:

            return ToolResponse::commit();
            break;

        case InToolActionID::LMB_RELEASE:
            if (m_is_transient)
                return ToolResponse::commit();
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
