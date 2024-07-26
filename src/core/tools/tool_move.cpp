#include "tool_move.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/entity/ientity_movable2d.hpp"
#include "document/entity/ientity_movable3d.hpp"
#include "document/entity/ientity_movable2d_initial_pos.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/iconstraint_workplane.hpp"
#include "document/constraint/iconstraint_movable.hpp"
#include "document/constraint/constraint_angle.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

ToolBase::CanBegin ToolMove::can_begin()
{
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            if (entity.can_move(get_doc()))
                return true;
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT) {
            auto &constr = get_doc().get_constraint(sr.item);
            if (dynamic_cast<const IConstraintMovable *>(&constr))
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
            m_inital_pos_wrkpl.emplace(uu, wrkpl.project(get_cursor_pos_for_workplane(wrkpl)));
        }
    }
    const Group *first_group = nullptr;
    for (const auto &sr : m_selection) {
        if (sr.type == SelectableRef::Type::ENTITY) {
            auto &entity = get_entity(sr.item);
            if (entity.m_move_instead.contains(sr.point)) {
                auto &enp = entity.m_move_instead.at(sr.point);
                auto &other_entity = get_entity(enp.entity);
                get_doc().accumulate_first_group(first_group, other_entity.m_group);
                m_entities.emplace(&other_entity, enp.point);
            }
            else {
                get_doc().accumulate_first_group(first_group, entity.m_group);
                m_entities.emplace(&entity, sr.point);
            }
        }
        else if (sr.type == SelectableRef::Type::CONSTRAINT) {
            if (auto constraint = get_doc().get_constraint_ptr<ConstraintLinesAngle>(sr.item)) {
                if (!constraint->m_wrkpl) {
                    auto vecs = constraint->get_vectors(get_doc());
                    m_inital_pos_angle_constraint.emplace(
                            sr.item, m_intf.get_cursor_pos_for_plane(constraint->get_origin(get_doc()), vecs.n));
                }
            }
        }
        // we don't care about constraints since dragging them is pureley cosmetic
    }
    if (first_group)
        m_first_group = first_group->m_uuid;


    for (auto [entity, point] : m_entities) {
        m_dragged_list.emplace_back(entity->m_uuid, point);
    }

    return ToolResponse();
}


ToolResponse ToolMove::update(const ToolArgs &args)
{
    auto &doc = get_doc();
    auto &last_doc = m_core.get_current_last_document();
    if (args.type == ToolEventType::MOVE) {
        const auto delta = m_intf.get_cursor_pos() - m_inital_pos;
        for (auto [entity, point] : m_entities) {
            if (!entity->can_move(doc))
                continue;
            if (auto en_movable = dynamic_cast<IEntityMovable2D *>(entity)) {
                const auto &wrkpl =
                        get_entity<EntityWorkplane>(dynamic_cast<const IEntityInWorkplane &>(*entity).get_workplane());
                const auto delta2d =
                        wrkpl.project(get_cursor_pos_for_workplane(wrkpl)) - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                auto &en_last = *last_doc.m_entities.at(entity->m_uuid);
                en_movable->move(en_last, delta2d, point);
            }
            else if (auto en_movable3d = dynamic_cast<IEntityMovable3D *>(entity)) {
                auto &en_last = *last_doc.m_entities.at(entity->m_uuid);
                en_movable3d->move(en_last, delta, point);
            }
            else if (auto en_movable_initial_pos = dynamic_cast<IEntityMovable2DIntialPos *>(entity)) {
                const auto &wrkpl =
                        get_entity<EntityWorkplane>(dynamic_cast<const IEntityInWorkplane &>(*entity).get_workplane());
                auto &en_last = *last_doc.m_entities.at(entity->m_uuid);
                en_movable_initial_pos->move(en_last, m_inital_pos_wrkpl.at(wrkpl.m_uuid),
                                             wrkpl.project(get_cursor_pos_for_workplane(wrkpl)), point);
            }
        }

        doc.set_group_solve_pending(m_first_group);
        m_core.solve_current(m_dragged_list);

        for (auto sr : m_selection) {
            if (sr.type == SelectableRef::Type::CONSTRAINT) {
                auto constraint = doc.m_constraints.at(sr.item).get();
                auto co_wrkpl = dynamic_cast<const IConstraintWorkplane *>(constraint);
                auto co_movable = dynamic_cast<IConstraintMovable *>(constraint);
                if (co_movable) {
                    auto cdelta = delta;
                    glm::dvec2 delta2d;
                    if (co_wrkpl) {
                        const auto wrkpl_uu = co_wrkpl->get_workplane(get_doc());
                        if (wrkpl_uu) {
                            auto &wrkpl = get_entity<EntityWorkplane>(wrkpl_uu);
                            delta2d = wrkpl.project(m_intf.get_cursor_pos_for_plane(wrkpl.m_origin,
                                                                                    wrkpl.get_normal_vector()))
                                      - m_inital_pos_wrkpl.at(wrkpl.m_uuid);
                            cdelta = wrkpl.transform_relative(delta2d);
                        }
                    }
                    auto &co_last = dynamic_cast<const IConstraintMovable &>(*last_doc.m_constraints.at(sr.item));
                    const auto odelta = (co_movable->get_origin(doc) - co_last.get_origin(last_doc));
                    if (co_movable->offset_is_in_workplane())
                        co_movable->set_offset(co_last.get_offset() + glm::dvec3(delta2d, 0) - odelta);
                    else
                        co_movable->set_offset(co_last.get_offset() + cdelta - odelta);
                }
            }
        }


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
