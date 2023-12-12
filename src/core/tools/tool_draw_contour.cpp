#include "tool_draw_contour.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_hv.hpp"
#include "document/constraint/constraint_arc_arc_tangent.hpp"
#include "document/constraint/constraint_arc_line_tangent.hpp"
#include "document/constraint/constraint_parallel.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "util/glm_util.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"
#include <iostream>


namespace dune3d {

ToolResponse ToolDrawContour::begin(const ToolArgs &args)
{
    m_wrkpl = get_workplane();
    m_intf.enable_hover_selection();
    if (m_tool_id == ToolID::DRAW_CONTOUR_FROM_POINT) {
        if (m_selection.size() != 1)
            return ToolResponse();
        auto &sr = *m_selection.begin();
        if (sr.type != SelectableRef::Type::ENTITY)
            return ToolResponse();
        auto &en = get_entity(sr.item);
        if ((en.get_type() != Entity::Type::LINE_2D) && (en.get_type() != Entity::Type::ARC_2D))
            return ToolResponse();
        if (sr.point == 0)
            return ToolResponse();
        auto enp = sr.get_entity_and_point();
        if (is_valid_tangent_point(enp))
            m_last_tangent_point = enp;
        m_temp_line = &add_entity<EntityLine2D>();
        m_temp_line->m_selection_invisible = true;
        m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
        m_temp_line->m_p1 = get_doc().get_point(enp);
        m_temp_line->m_p2 = get_cursor_pos_in_plane();
        m_entities.push_back(m_temp_line);
        {
            auto &constraint = add_constraint<ConstraintPointsCoincident>();
            constraint.m_wrkpl = m_wrkpl->m_uuid;
            constraint.m_entity1 = enp;
            constraint.m_entity2 = {m_temp_line->m_uuid, 1};
            m_constraints.insert(&constraint);
        }
        update_constrain_tangent();
    }

    return ToolResponse();
}

bool ToolDrawContour::can_begin()
{
    return get_workplane_uuid() != UUID();
}

glm::dvec2 ToolDrawContour::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(m_intf.get_cursor_pos_for_plane(m_wrkpl->m_origin, m_wrkpl->get_normal_vector()));
}

glm::dvec2 ToolDrawContour::get_last_tangent()
{
    auto &enp = m_last_tangent_point.value();
    auto &en = get_entity(enp.entity);
    if (auto en_tangent = dynamic_cast<const IEntityTangent *>(&en)) {
        return en_tangent->get_tangent_at_point(enp.point);
    }
    else {
        throw std::runtime_error("entity has no tangent");
    }
}

unsigned int ToolDrawContour::get_arc_tail_point() const
{
    if (m_flip_arc)
        return 2;
    else
        return 1;
}

unsigned int ToolDrawContour::get_arc_head_point() const
{
    if (m_flip_arc)
        return 1;
    else
        return 2;
}

void ToolDrawContour::update_arc_center()
{
    if (m_last_tangent_point && m_constrain_tangent) {
        glm::dvec2 origin;
        origin = m_temp_arc->get_point_in_workplane(get_arc_tail_point());
        auto v = m_temp_arc->get_point_in_workplane(get_arc_head_point()) - origin;
        auto last_tangent = get_last_tangent();
        const auto ortho = glm::normalize(glm::dvec2(-last_tangent.y, last_tangent.x));
        const auto o = glm::dot(v, ortho);

        auto c = glm::cross(glm::dvec3(v, 0), glm::dvec3(last_tangent, 0)).z;
        set_flip_arc(c > 0);

        const auto a = pow(glm::length(v), 2) / (2 * o);
        m_temp_arc->m_center = origin + a * ortho;
    }
    else {
        m_temp_arc->m_center = (m_temp_arc->m_from + m_temp_arc->m_to) / 2.;
    }
}

bool ToolDrawContour::is_valid_tangent_point(const EntityAndPoint &enp)
{
    const auto &en = get_doc().get_entity(enp.entity);
    if (!en.is_valid_point(enp.point))
        return false;
    return dynamic_cast<const IEntityTangent *>(&en);
}

ToolResponse ToolDrawContour::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_temp_line) {
            if (m_constrain_tangent && m_last_tangent_point) {
                const auto last_tangent = glm::normalize(get_last_tangent());
                auto v = get_cursor_pos_in_plane() - m_temp_line->m_p1;
                const auto d = glm::dot(last_tangent, v);
                m_temp_line->m_p2 = m_temp_line->m_p1 + last_tangent * d;
            }
            else {
                m_temp_line->m_p2 = get_cursor_pos_in_plane();
            }
        }
        else if (m_temp_arc) {
            if (m_placing_center) {
                m_temp_arc->m_center =
                        project_onto_perp_bisector(m_temp_arc->m_from, m_temp_arc->m_to, get_cursor_pos_in_plane());
            }
            else {
                if (m_flip_arc)
                    m_temp_arc->m_from = get_cursor_pos_in_plane();
                else
                    m_temp_arc->m_to = get_cursor_pos_in_plane();
                update_arc_center();
            }
        }
        update_tip();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
        case InToolActionID::LMB_DOUBLE:
            if (m_temp_arc && !m_placing_center && !(m_constrain_tangent && m_last_tangent_point)) {
                m_placing_center = true;
                update_tip();
                return ToolResponse();
            }
            m_placing_center = false;

            if (get_temp_entity()) {
                if (m_last == get_cursor_pos_in_plane())
                    return end_tool();
            }

            m_last = get_cursor_pos_in_plane();

            if (m_temp_line) {
                if (auto ct = get_auto_constraint()) {
                    ConstraintHV *constraint = nullptr;
                    if (*ct == Constraint::Type::HORIZONTAL) {
                        constraint = &add_constraint<ConstraintHorizontal>();
                        m_temp_line->m_p2.y = m_temp_line->m_p1.y;
                    }
                    else if (*ct == Constraint::Type::VERTICAL) {
                        constraint = &add_constraint<ConstraintVertical>();
                        m_temp_line->m_p2.x = m_temp_line->m_p1.x;
                    }
                    assert(constraint);
                    m_constraints.insert(constraint);
                    constraint->m_entity1 = {m_temp_line->m_uuid, 1};
                    constraint->m_entity2 = {m_temp_line->m_uuid, 2};
                    constraint->m_wrkpl = m_wrkpl->m_uuid;
                }
            }

            auto last_point = get_last_point();


            if (m_temp_line) {
                if (m_last_tangent_point && m_constrain_tangent) {
                    auto &last_tangent_entity = get_entity(m_last_tangent_point->entity);
                    if (last_tangent_entity.get_type() == Entity::Type::LINE_2D) {
                        auto &constraint = add_constraint<ConstraintParallel>();
                        constraint.m_entity1 = last_tangent_entity.m_uuid;
                        constraint.m_entity2 = m_temp_line->m_uuid;
                        constraint.m_wrkpl = m_wrkpl->m_uuid;
                        m_constraints.insert(&constraint);
                    }
                    else if (last_tangent_entity.get_type() == Entity::Type::ARC_2D) {
                        auto &constraint = add_constraint<ConstraintArcLineTangent>();
                        constraint.m_arc = m_last_tangent_point.value();
                        constraint.m_line = m_temp_line->m_uuid;
                        m_constraints.insert(&constraint);
                    }
                }
                m_last_tangent_point = {m_temp_line->m_uuid, 2};
            }
            else if (m_temp_arc) {
                // add tangent constraint
                if (m_last_tangent_point && m_constrain_tangent) {
                    auto &last_tangent_entity = get_entity(m_last_tangent_point->entity);
                    if (last_tangent_entity.get_type() == Entity::Type::LINE_2D) {
                        auto &constraint = add_constraint<ConstraintArcLineTangent>();
                        constraint.m_line = last_tangent_entity.m_uuid;
                        constraint.m_arc.entity = m_temp_arc->m_uuid;
                        constraint.m_arc.point = get_arc_tail_point();
                        m_constraints.insert(&constraint);
                    }
                    else if (last_tangent_entity.get_type() == Entity::Type::ARC_2D) {
                        auto &constraint = add_constraint<ConstraintArcArcTangent>();
                        constraint.m_arc1 = m_last_tangent_point.value();
                        constraint.m_arc2.entity = m_temp_arc->m_uuid;
                        constraint.m_arc2.point = get_arc_tail_point();
                        m_constraints.insert(&constraint);
                    }
                }

                m_last_tangent_point = {m_temp_arc->m_uuid, get_arc_head_point()};
            }

            if (m_constrain && m_entities.size()) {
                if (constrain_point(m_wrkpl->m_uuid, {m_entities.back()->m_uuid, last_point})) {
                    return ToolResponse::commit();
                }
            }

            m_temp_line = &add_entity<EntityLine2D>();
            m_temp_arc = nullptr;
            m_temp_line->m_selection_invisible = true;
            m_temp_line->m_p1 = get_cursor_pos_in_plane();
            if (m_entities.size()) {
                if (auto last_line = dynamic_cast<const EntityLine2D *>(m_entities.back())) {
                    m_temp_line->m_p1 = last_line->m_p2;
                }
                else if (auto last_arc = dynamic_cast<const EntityArc2D *>(m_entities.back())) {
                    m_temp_line->m_p1 = last_arc->get_point_in_workplane(last_point);
                }
            }

            m_temp_line->m_p2 = get_cursor_pos_in_plane();
            m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
            if (m_entities.size())
                m_temp_line->m_construction = m_entities.back()->m_construction;

            if (m_constrain) {
                if (m_entities.size() == 0) {
                    if (auto constraint = constrain_point(m_wrkpl->m_uuid, {m_temp_line->m_uuid, 1})) {
                        auto enp = m_intf.get_hover_selection().value().get_entity_and_point();
                        if (is_valid_tangent_point(enp)) {
                            m_last_tangent_point = enp;
                        }
                        m_constraints.insert(constraint);
                    }
                }
            }
            update_constrain_tangent();

            if (m_entities.size()) {
                auto &last_ent = *m_entities.back();
                last_ent.m_selection_invisible = false;
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                m_constraints.insert(&constraint);
                constraint.m_entity1 = {m_entities.back()->m_uuid, last_point};
                constraint.m_entity2 = {m_temp_line->m_uuid, 1};
                constraint.m_wrkpl = m_wrkpl->m_uuid;
            }

            m_entities.push_back(m_temp_line);
        }

        break;


        case InToolActionID::TOGGLE_CONSTRUCTION: {
            if (auto t = get_temp_entity()) {
                t->m_construction = !t->m_construction;
                for (auto en : m_entities) {
                    en->m_construction = t->m_construction;
                }
            }
        } break;

        case InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT: {
            m_constrain = !m_constrain;
        } break;

        case InToolActionID::TOGGLE_HV_CONSTRAINT: {
            if (m_temp_line)
                m_constrain_hv = !m_constrain_hv;
        } break;

        case InToolActionID::TOGGLE_TANGENT_CONSTRAINT: {
            m_constrain_tangent = !m_constrain_tangent;
        } break;

        case InToolActionID::TOGGLE_ARC: {
            if (get_temp_entity()) {
                if (m_temp_line) {
                    m_constrain_tangent = true;
                    m_temp_arc = &add_entity<EntityArc2D>();
                    if (m_flip_arc) {
                        m_temp_arc->m_to = m_temp_line->m_p1;
                        m_temp_arc->m_from = m_temp_line->m_p2;
                    }
                    else {
                        m_temp_arc->m_from = m_temp_line->m_p1;
                        m_temp_arc->m_to = m_temp_line->m_p2;
                    }
                    update_arc_center();
                    m_temp_arc->m_wrkpl = m_wrkpl->m_uuid;
                    m_temp_arc->m_selection_invisible = true;
                    for (auto constraint : m_constraints) {
                        for (unsigned int pt = 1; pt <= 2; pt++) {
                            unsigned int arc_pt = pt;
                            if (m_flip_arc)
                                arc_pt = 3 - pt;
                            constraint->replace_point({m_temp_line->m_uuid, pt}, {m_temp_arc->m_uuid, arc_pt});
                        }
                    }
                    get_doc().m_entities.erase(m_temp_line->m_uuid);
                    m_temp_line = nullptr;
                    m_entities.back() = m_temp_arc;
                }
                else if (m_temp_arc) {
                    m_constrain_tangent = false;
                    m_temp_line = &add_entity<EntityLine2D>();
                    m_temp_line->m_p1 = m_temp_arc->get_point_in_workplane(get_arc_tail_point());
                    m_temp_line->m_p2 = m_temp_arc->get_point_in_workplane(get_arc_head_point());
                    m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
                    m_temp_line->m_selection_invisible = true;
                    for (auto constraint : m_constraints) {
                        for (unsigned int pt = 1; pt <= 2; pt++) {
                            unsigned int arc_pt = pt;
                            if (m_flip_arc)
                                arc_pt = 3 - pt;
                            constraint->replace_point({m_temp_arc->m_uuid, arc_pt}, {m_temp_line->m_uuid, pt});
                        }
                    }
                    get_doc().m_entities.erase(m_temp_arc->m_uuid);
                    m_temp_arc = nullptr;
                    m_entities.back() = m_temp_line;
                }
            }
        } break;

        case InToolActionID::FLIP_ARC: {
            if (m_temp_arc && !(m_last_tangent_point && m_constrain_tangent)) {
                set_flip_arc(!m_flip_arc);
            }
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL: {
            auto r = end_tool();
            if (r.result != ToolResponse::Result::NOP)
                return r;
        } break;


        default:;
        }
    }
    update_tip();
    return ToolResponse();
}

void ToolDrawContour::update_constrain_tangent()
{
    if (m_last_tangent_point && get_entity(m_last_tangent_point->entity).get_type() == Entity::Type::LINE_2D)
        m_constrain_tangent = false;
}

void ToolDrawContour::set_flip_arc(bool flip)
{
    if (flip == m_flip_arc)
        return;

    std::swap(m_temp_arc->m_from, m_temp_arc->m_to);
    for (auto constraint : m_constraints) {
        constraint->replace_point({m_temp_arc->m_uuid, 1}, {m_temp_arc->m_uuid, 10});
        constraint->replace_point({m_temp_arc->m_uuid, 2}, {m_temp_arc->m_uuid, 1});
        constraint->replace_point({m_temp_arc->m_uuid, 10}, {m_temp_arc->m_uuid, 2});
    }
    m_flip_arc = flip;
}

unsigned int ToolDrawContour::get_last_point() const
{
    unsigned int last_point = 2;
    if (m_temp_arc && m_flip_arc)
        last_point = 1;
    return last_point;
}

ToolResponse ToolDrawContour::end_tool()
{
    if (auto t = get_temp_entity()) {
        for (auto constraint : m_constraints) {
            auto ents = constraint->get_referenced_entities();
            if (ents.contains(t->m_uuid))
                get_doc().m_constraints.erase(constraint->m_uuid);
        }
        get_doc().m_entities.erase(t->m_uuid);
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
    return ToolResponse();
}

void ToolDrawContour::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    if (m_placing_center)
        actions.emplace_back(InToolActionID::LMB, "place center");
    else
        actions.emplace_back(InToolActionID::LMB, "place");

    actions.emplace_back(InToolActionID::RMB, "end tool");


    if (auto t = get_temp_entity()) {
        if (t->m_construction)
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "normal");
        else
            actions.emplace_back(InToolActionID::TOGGLE_CONSTRUCTION, "construction");
    }

    if (m_constrain)
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "coincident constraint off");
    else
        actions.emplace_back(InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT, "coincident constraint on");

    if (m_temp_line) {
        if (m_constrain_hv)
            actions.emplace_back(InToolActionID::TOGGLE_HV_CONSTRAINT, "h/v constraint off");
        else
            actions.emplace_back(InToolActionID::TOGGLE_HV_CONSTRAINT, "h/v constraint on");
    }

    if (m_last_tangent_point) {
        if (m_constrain_tangent)
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "tangent constraint off");
        else
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "tangent constraint on");
    }

    if (get_temp_entity())
        actions.emplace_back(InToolActionID::TOGGLE_ARC);

    if (m_temp_arc && !(m_constrain_tangent && m_last_tangent_point))
        actions.emplace_back(InToolActionID::FLIP_ARC);

    std::string tip;
    if (m_constrain) {
        std::string what = "to";
        if (m_placing_center)
            what = "center";
        tip += get_constrain_tip(what);
    }

    if (auto ct = get_auto_constraint()) {
        if (*ct == Constraint::Type::HORIZONTAL)
            tip += " horizontal";
        else if (*ct == Constraint::Type::VERTICAL)
            tip += " vertical";
    }
    if (!tip.size())
        tip = " ";
    m_intf.tool_bar_set_tool_tip(tip);

    m_intf.tool_bar_set_actions(actions);
}

std::optional<Constraint::Type> ToolDrawContour::get_auto_constraint() const
{
    if (!m_temp_line)
        return {};
    if (!m_constrain_hv)
        return {};

    auto v = glm::abs(m_temp_line->m_p1 - m_temp_line->m_p2);
    auto angle = glm::degrees(atan2(v.y, v.x));
    if (angle < 10)
        return Constraint::Type::HORIZONTAL;
    else if (angle > 80)
        return Constraint::Type::VERTICAL;
    else
        return {};
}

Entity *ToolDrawContour::get_temp_entity()
{
    if (m_temp_line)
        return m_temp_line;
    if (m_temp_arc)
        return m_temp_arc;
    return nullptr;
}

} // namespace dune3d
