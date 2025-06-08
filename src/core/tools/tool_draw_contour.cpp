#include "tool_draw_contour.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_hv.hpp"
#include "document/constraint/constraint_arc_arc_tangent.hpp"
#include "document/constraint/constraint_arc_line_tangent.hpp"
#include "document/constraint/constraint_parallel.hpp"
#include "document/constraint/constraint_bezier_line_tangent.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "util/glm_util.hpp"
#include "util/template_util.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"
#include "document/solid_model/solid_model_util.hpp"


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
        if (!en.of_type(Entity::Type::LINE_2D, Entity::Type::ARC_2D, Entity::Type::BEZIER_2D))
            return ToolResponse();
        if (sr.point == 0)
            return ToolResponse();
        auto enp = sr.get_entity_and_point();
        if (is_valid_tangent_point(enp))
            m_last_tangent_point = enp;
        m_temp_line = &add_entity<EntityLine2D>();
        m_temp_line->m_selection_invisible = true;
        m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
        m_temp_line->m_p1 = m_wrkpl->project(get_doc().get_point(enp));
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
    update_tip();

    return ToolResponse();
}

ToolBase::CanBegin ToolDrawContour::can_begin()
{
    return get_workplane_uuid() != UUID();
}

bool ToolDrawContour::is_draw_contour() const
{
    return any_of(m_tool_id, ToolID::DRAW_CONTOUR, ToolID::DRAW_CONTOUR_FROM_POINT);
}

glm::dvec2 ToolDrawContour::get_cursor_pos_in_plane() const
{
    return m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl));
}

glm::dvec2 ToolDrawContour::get_tangent_for_point(const EntityAndPoint &enp)
{
    auto &en = get_entity(enp.entity);
    if (auto en_tangent = dynamic_cast<const IEntityTangent *>(&en)) {
        if (en_tangent->is_valid_tangent_point(enp.point))
            return en_tangent->get_tangent_at_point(enp.point);
        else
            throw std::runtime_error("not a valid tangent point");
    }
    else {
        throw std::runtime_error("entity has no tangent");
    }
}
glm::dvec2 ToolDrawContour::get_last_tangent()
{
    auto &enp = m_last_tangent_point.value();
    return get_tangent_for_point(enp);
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

unsigned int ToolDrawContour::get_head_point() const
{
    if (m_temp_arc)
        return get_arc_head_point();
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

void ToolDrawContour::update_bezier_controls()
{
    if (m_last_tangent_point && m_constrain_tangent) {
        auto last_tangent = glm::normalize(get_last_tangent());
        m_temp_bezier->m_c1 =
                m_temp_bezier->m_p1 + 0.5 * last_tangent * glm::length(m_temp_bezier->m_p1 - m_temp_bezier->m_p2);
    }
    else {
        m_temp_bezier->m_c1 = m_temp_bezier->m_p1;
    }
}

bool ToolDrawContour::is_valid_tangent_point(const EntityAndPoint &enp)
{
    const auto &en = get_doc().get_entity(enp.entity);
    if (!en.is_valid_point(enp.point))
        return false;
    auto en_tangent = dynamic_cast<const IEntityTangent *>(&en);
    if (!en_tangent)
        return false;
    if (!en_tangent->is_valid_tangent_point(enp.point))
        return false;
    // if the point already has a coincident constraint, it's not possible to tell
    // which one got selected, so the tangent would be arbitrary
    // so no tangent for us then
    for (auto constraint : en.get_constraints(get_doc())) {
        if (auto cc = dynamic_cast<const ConstraintPointsCoincident *>(constraint)) {
            if (cc->m_entity1 == enp || cc->m_entity2 == enp)
                return false;
        }
    }
    return true;
}

static double sq(double x)
{
    return x * x;
};

Constraint *ToolDrawContour::constrain_point_and_add_head_tangent_constraint(const UUID &wrkpl,
                                                                             const EntityAndPoint &enp_to_constrain)
{
    auto coincident_constraint = constrain_point(wrkpl, enp_to_constrain);
    if (coincident_constraint) {
        if (coincident_constraint->of_type(Constraint::Type::POINTS_COINCIDENT) && m_has_tangent_head
            && m_constrain_tangent_head) {
            auto &en_head = *get_temp_entity();
            const auto enp_head = EntityAndPoint{en_head.m_uuid, get_head_point()};
            const auto enp_target = m_intf.get_hover_selection().value().get_entity_and_point();
            auto &en_target = get_entity(enp_target.entity);
            const auto head_constraint_type = get_head_constraint(en_head, en_target);
            if (head_constraint_type) {
                const auto redundant_before = current_group_has_redundant_constraints();
                Constraint *new_constraint = nullptr;
                switch (*head_constraint_type) {
                case Constraint::Type::ARC_LINE_TANGENT: {
                    auto &constraint = add_constraint<ConstraintArcLineTangent>();
                    if (en_head.of_type(Entity::Type::ARC_2D)) {
                        constraint.m_line = en_target.m_uuid;
                        constraint.m_arc = enp_head;
                    }
                    else {
                        constraint.m_line = en_head.m_uuid;
                        constraint.m_arc = enp_target;
                    }
                    new_constraint = &constraint;
                } break;
                case Constraint::Type::ARC_ARC_TANGENT: {
                    auto &constraint = add_constraint<ConstraintArcArcTangent>();
                    constraint.m_arc1 = enp_head;
                    constraint.m_arc2 = enp_target;
                    new_constraint = &constraint;
                } break;
                case Constraint::Type::PARALLEL: {
                    auto &constraint = add_constraint<ConstraintParallel>();
                    constraint.m_modify_to_satisfy = true;
                    constraint.m_entity1 = enp_head.entity;
                    constraint.m_entity2 = enp_target.entity;
                    constraint.m_wrkpl = get_workplane_uuid();
                    new_constraint = &constraint;
                } break;
                default:;
                }
                if (!redundant_before) {
                    const auto redundant_after = current_group_has_redundant_constraints();
                    if (redundant_after) {
                        get_doc().m_constraints.erase(new_constraint->m_uuid);
                        new_constraint = nullptr;
                    }
                }
                if (new_constraint)
                    m_constraints.insert(new_constraint);
            }
        }
    }
    return coincident_constraint;
}

bool ToolDrawContour::check_close_path()
{
    if (auto hsel = m_intf.get_hover_selection()) {
        const auto paths =
                solid_model_util::Paths::from_document(get_doc(), m_wrkpl->m_uuid, m_core.get_current_group());
        for (const auto &path : paths.paths) {
            if (std::ranges::find_if(
                        path, [this](const auto &x) { return x.second.entity.m_uuid == m_entities.back()->m_uuid; })
                        != path.end()
                && std::ranges::find_if(path, [&hsel](const auto &x) { return x.second.entity.m_uuid == hsel->item; })
                           != path.end()) {
                return true;
            }
        }
    }
    return false;
}

template <typename T> class AutoReset {
public:
    [[nodiscard]]
    AutoReset(T &v, T v_set)
        : m_v(v), m_v_set(v_set)
    {
    }

    ~AutoReset()
    {
        m_v = m_v_set;
    }

private:
    T &m_v;
    const T m_v_set;
};

ToolResponse ToolDrawContour::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        m_tangent_valid = true;
        if (m_temp_line) {
            if (m_constrain_tangent && m_last_tangent_point) {
                if (m_entities.size() > 1
                    && m_last_tangent_point->entity == m_entities.at(m_entities.size() - 2)->m_uuid
                    && m_entities.at(m_entities.size() - 2)->of_type(Entity::Type::ARC_2D)) {
                    m_temp_line->m_p2 = get_cursor_pos_in_plane();
                    auto &arc = dynamic_cast<EntityArc2D &>(*m_entities.at(m_entities.size() - 2));
                    const auto c = arc.m_center;
                    const auto other_pt = m_last_tangent_point->point == 1 ? 2 : 1;
                    // https://en.wikipedia.org/wiki/Tangent_lines_to_circles#With_analytic_geometry
                    const auto r = glm::length(arc.get_point_in_workplane(other_pt) - c);
                    const auto p = m_temp_line->m_p2 - c;
                    const auto d = glm::length(p);
                    if (d > r) {
                        const auto a = c + sq(r) / sq(d) * p;
                        const auto b = r / sq(d) * sqrt(sq(d) - sq(r)) * glm::dvec2(-p.y, p.x);
                        const auto q1 = a + b;
                        const auto q2 = a - b;
                        const auto qlast = arc.get_point_in_workplane(m_last_tangent_point->point);
                        const auto q1d = glm::length(q1 - qlast);
                        const auto q2d = glm::length(q2 - qlast);
                        const auto q = (q1d < q2d) ? q1 : q2;
                        m_temp_line->m_p1 = q;
                        if (m_last_tangent_point->point == 1)
                            arc.m_from = q;
                        else
                            arc.m_to = q;
                    }
                    else {
                        m_tangent_valid = false;
                    }
                }
                else if (m_entities.size() > 1
                         && m_last_tangent_point->entity == m_entities.at(m_entities.size() - 2)->m_uuid
                         && m_entities.at(m_entities.size() - 2)->of_type(Entity::Type::BEZIER_2D)) {
                    m_temp_line->m_p2 = get_cursor_pos_in_plane();
                    auto &bez = dynamic_cast<EntityBezier2D &>(*m_entities.at(m_entities.size() - 2));
                    const auto c_len = glm::length(bez.m_c2 - bez.m_p2);
                    const auto v = m_temp_line->m_p2 - m_temp_line->m_p1;
                    bez.m_c2 = bez.m_p2 - glm::normalize(v) * c_len;
                }
                else {
                    const auto last_tangent = glm::normalize(get_last_tangent());
                    auto v = get_cursor_pos_in_plane() - m_temp_line->m_p1;
                    const auto d = glm::dot(last_tangent, v);
                    m_temp_line->m_p2 = m_temp_line->m_p1 + last_tangent * d;
                }
            }
            else {
                m_temp_line->m_p2 = get_cursor_pos_in_plane();
            }
        }
        else if (m_temp_arc) {
            if (is_placing_center()) {
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
        else if (m_temp_bezier) {
            if (m_state == State::NORMAL) {
                m_temp_bezier->m_p2 = get_cursor_pos_in_plane();
                m_temp_bezier->m_c2 = m_temp_bezier->m_p2;
                update_bezier_controls();
            }
            else if (m_state == State::BEZIER_C1) {
                if (m_last_tangent_point && m_constrain_tangent) {
                    // project onto tangent
                    auto t = glm::normalize(get_last_tangent());
                    auto v = get_cursor_pos_in_plane() - m_temp_bezier->m_p1;
                    m_temp_bezier->m_c1 = m_temp_bezier->m_p1 + t * glm::dot(t, v);
                }
                else {
                    m_temp_bezier->m_c1 = get_cursor_pos_in_plane();
                }
            }
            else if (m_state == State::BEZIER_C2) {
                if (m_bezier_head_tangent_point && m_constrain_tangent) {
                    // project onto tangent
                    auto t = glm::normalize(get_tangent_for_point(m_bezier_head_tangent_point.value()));
                    auto v = get_cursor_pos_in_plane() - m_temp_bezier->m_p2;
                    m_temp_bezier->m_c2 = m_temp_bezier->m_p2 + t * glm::dot(t, v);
                }
                else {
                    m_temp_bezier->m_c2 = get_cursor_pos_in_plane();
                }
            }
        }
        update_tip();
        set_first_update_group_current();
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            if (m_temp_arc && !is_placing_center() && !(m_constrain_tangent && m_last_tangent_point)) {
                if (m_constrain) {
                    if (constrain_point_and_add_head_tangent_constraint(m_wrkpl->m_uuid,
                                                                        {m_temp_arc->m_uuid, get_last_point()}))
                        m_close_path = check_close_path();
                }
                m_state = State::CENTER;
                update_tip();
                return ToolResponse();
            }
            if (m_temp_bezier) {
                if (m_state == State::NORMAL) {
                    if (m_constrain) {
                        if (constrain_point_and_add_head_tangent_constraint(m_wrkpl->m_uuid,
                                                                            {m_temp_bezier->m_uuid, get_last_point()}))
                            m_close_path = check_close_path();
                    }
                    m_bezier_head_tangent_point.reset();
                    if (auto hsel = m_intf.get_hover_selection()) {
                        auto enp = hsel->get_entity_and_point();
                        auto &en = get_entity(enp.entity);
                        if (auto en_tangent = dynamic_cast<const IEntityTangent *>(&en)) {
                            if (en_tangent->is_valid_tangent_point(enp.point)) {
                                m_bezier_head_tangent_point = enp;
                            }
                        }
                    }
                    m_state = State::BEZIER_C1;
                    update_tip();
                    return ToolResponse();
                }
                else if (m_state == State::BEZIER_C1) {
                    if (m_constrain) {
                        constrain_point_and_add_head_tangent_constraint(m_wrkpl->m_uuid, {m_temp_bezier->m_uuid, 3});
                    }
                    if (m_last_tangent_point && m_constrain_tangent) {
                        auto &last_tangent_entity = get_entity(m_last_tangent_point->entity);
                        if (last_tangent_entity.get_type() == Entity::Type::LINE_2D) {
                            auto &constraint = add_constraint<ConstraintBezierLineTangent>();
                            constraint.m_line = last_tangent_entity.m_uuid;
                            constraint.m_bezier = {m_temp_bezier->m_uuid, 1};
                            m_constraints.insert(&constraint);
                        }
                        else if (last_tangent_entity.of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D)) {
                            auto &constraint = add_constraint<ConstraintArcArcTangent>();
                            constraint.m_arc1 = m_last_tangent_point.value();
                            constraint.m_arc2 = {m_temp_bezier->m_uuid, 1};
                            m_constraints.insert(&constraint);
                        }
                    }
                    m_state = State::BEZIER_C2;
                    update_tip();
                    return ToolResponse();
                }
                else if (m_state == State::BEZIER_C2) {
                    m_constrain_tangent = false;
                    update_tip();
                }
            }
            AutoReset autoreset{m_state, State::NORMAL};

            if (get_temp_entity()) {
                if (m_last == get_cursor_pos_in_plane())
                    return end_tool();
            }

            m_last = get_cursor_pos_in_plane();


            auto last_point = get_last_point();


            if (m_temp_line) {
                if (m_last_tangent_point && m_constrain_tangent && m_tangent_valid) {
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
                    else if (last_tangent_entity.get_type() == Entity::Type::BEZIER_2D) {
                        auto &constraint = add_constraint<ConstraintBezierLineTangent>();
                        constraint.m_bezier = m_last_tangent_point.value();
                        constraint.m_line = m_temp_line->m_uuid;
                        m_constraints.insert(&constraint);
                    }
                }
                m_last_tangent_point = {m_temp_line->m_uuid, 2};
            }
            else if (m_temp_arc) {
                // add tangent constraint
                if (m_last_tangent_point && m_constrain_tangent && m_tangent_valid) {
                    auto &last_tangent_entity = get_entity(m_last_tangent_point->entity);
                    if (last_tangent_entity.get_type() == Entity::Type::LINE_2D) {
                        auto &constraint = add_constraint<ConstraintArcLineTangent>();
                        constraint.m_line = last_tangent_entity.m_uuid;
                        constraint.m_arc.entity = m_temp_arc->m_uuid;
                        constraint.m_arc.point = get_arc_tail_point();
                        m_constraints.insert(&constraint);
                    }
                    else if (last_tangent_entity.of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D)) {
                        auto &constraint = add_constraint<ConstraintArcArcTangent>();
                        constraint.m_arc1 = m_last_tangent_point.value();
                        constraint.m_arc2.entity = m_temp_arc->m_uuid;
                        constraint.m_arc2.point = get_arc_tail_point();
                        m_constraints.insert(&constraint);
                    }
                }

                m_last_tangent_point = {m_temp_arc->m_uuid, get_arc_head_point()};
            }
            else if (m_temp_bezier) {
                if (m_bezier_head_tangent_point && m_constrain_tangent) {
                    auto &tangent_entity = get_entity(m_bezier_head_tangent_point->entity);
                    if (tangent_entity.of_type(Entity::Type::LINE_2D)) {
                        auto &constraint = add_constraint<ConstraintBezierLineTangent>();
                        constraint.m_line = tangent_entity.m_uuid;
                        constraint.m_bezier = {m_temp_bezier->m_uuid, 2};
                        m_constraints.insert(&constraint);
                    }
                    else if (tangent_entity.of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D)) {
                        auto &constraint = add_constraint<ConstraintArcArcTangent>();
                        constraint.m_arc1 = m_bezier_head_tangent_point.value();
                        constraint.m_arc2 = {m_temp_bezier->m_uuid, 2};
                        m_constraints.insert(&constraint);
                    }
                }

                m_last_tangent_point = {m_temp_bezier->m_uuid, 2};
            }


            if (m_constrain && m_entities.size()) {
                auto point = last_point;
                switch (m_state) {
                case State::CENTER:
                    point = 3;
                    break;
                case State::BEZIER_C1:
                    point = 3;
                    break;
                case State::BEZIER_C2:
                    point = 4;
                    break;
                default:;
                }
                if (constrain_point_and_add_head_tangent_constraint(m_wrkpl->m_uuid,
                                                                    {m_entities.back()->m_uuid, point})) {
                    if (m_state == State::NORMAL)
                        m_close_path = check_close_path();
                }
            }

            if (auto ct = get_auto_constraint()) {
                const auto redundant_before = current_group_has_redundant_constraints();
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
                constraint->m_entity1 = {m_temp_line->m_uuid, 1};
                constraint->m_entity2 = {m_temp_line->m_uuid, 2};
                constraint->m_wrkpl = m_wrkpl->m_uuid;
                if (!redundant_before) {
                    const auto redundant_after = current_group_has_redundant_constraints();
                    if (redundant_after) {
                        get_doc().m_constraints.erase(constraint->m_uuid);
                        constraint = nullptr;
                    }
                }
                if (constraint)
                    m_constraints.insert(constraint);
            }


            if (m_close_path)
                return ToolResponse::commit();

            if (m_entities.size() && !is_draw_contour())
                return ToolResponse::commit();

            if (m_entities.size() == 0) {
                auto hsel = m_intf.get_hover_selection();
                if (hsel) {
                    const auto enp = hsel->get_entity_and_point();
                    if (is_valid_tangent_point(enp))
                        m_last_tangent_point = enp;
                }
            }

            if (m_tool_id == ToolID::DRAW_ARC_2D) {
                m_temp_line = nullptr;
                m_temp_bezier = nullptr;
                m_temp_arc = &add_entity<EntityArc2D>();
                m_temp_arc->m_selection_invisible = true;
                m_temp_arc->m_from = get_cursor_pos_in_plane();

                if (m_entities.size()) {
                    if (auto last_line = dynamic_cast<const EntityLine2D *>(m_entities.back())) {
                        m_temp_arc->m_from = last_line->m_p2;
                    }
                    else if (auto last_arc = dynamic_cast<const EntityArc2D *>(m_entities.back())) {
                        m_temp_arc->m_from = last_arc->get_point_in_workplane(last_point);
                    }
                    else if (auto last_bez = dynamic_cast<const EntityBezier2D *>(m_entities.back())) {
                        m_temp_arc->m_from = last_bez->m_p2;
                    }
                }
                m_temp_arc->m_center = m_temp_arc->m_from + glm::dvec2(1, 0);
                m_temp_arc->m_to = m_temp_arc->m_from + glm::dvec2(2, 0);
                m_temp_arc->m_wrkpl = m_wrkpl->m_uuid;
            }
            else if (m_tool_id == ToolID::DRAW_BEZIER_2D) {
                m_temp_line = nullptr;
                m_temp_arc = nullptr;
                m_temp_bezier = &add_entity<EntityBezier2D>();
                m_temp_bezier->m_selection_invisible = true;
                m_temp_bezier->m_p1 = get_cursor_pos_in_plane();
                m_temp_bezier->m_p2 = m_temp_bezier->m_p1 + glm::dvec2(3, 0);
                m_temp_bezier->m_c1 = m_temp_bezier->m_p1 + glm::dvec2(1, 0);
                m_temp_bezier->m_c2 = m_temp_bezier->m_p1 + glm::dvec2(2, 0);
                m_temp_bezier->m_wrkpl = m_wrkpl->m_uuid;
            }
            else {
                m_temp_line = &add_entity<EntityLine2D>();
                m_temp_arc = nullptr;
                m_temp_bezier = nullptr;
                m_temp_line->m_selection_invisible = true;
                m_temp_line->m_p1 = get_cursor_pos_in_plane();
                if (m_entities.size()) {
                    if (auto last_line = dynamic_cast<const EntityLine2D *>(m_entities.back())) {
                        m_temp_line->m_p1 = last_line->m_p2;
                    }
                    else if (auto last_arc = dynamic_cast<const EntityArc2D *>(m_entities.back())) {
                        m_temp_line->m_p1 = last_arc->get_point_in_workplane(last_point);
                    }
                    else if (auto last_bez = dynamic_cast<const EntityBezier2D *>(m_entities.back())) {
                        m_temp_line->m_p1 = last_bez->m_p2;
                    }
                    m_temp_line->m_construction = m_entities.back()->m_construction;
                }
                m_temp_line->m_p2 = get_cursor_pos_in_plane();
                m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
            }
            m_close_path = false;


            if (m_constrain) {
                if (m_entities.size() == 0) {
                    if (auto constraint = constrain_point(m_wrkpl->m_uuid, {get_temp_entity()->m_uuid, 1})) {
                        auto enp = m_intf.get_hover_selection().value().get_entity_and_point();
                        if (is_valid_tangent_point(enp)) {
                            m_last_tangent_point = enp;
                        }
                        m_constraints.insert(constraint);
                    }
                }
            }
            if (m_temp_arc) {
                m_temp_arc->m_center = m_temp_arc->m_from;
                m_temp_arc->m_to = m_temp_arc->m_from;
            }

            update_constrain_tangent();

            if (m_entities.size()) {
                auto &last_ent = *m_entities.back();
                last_ent.m_selection_invisible = false;
                auto &constraint = add_constraint<ConstraintPointsCoincident>();
                m_constraints.insert(&constraint);
                constraint.m_entity1 = {m_entities.back()->m_uuid, last_point};
                constraint.m_entity2 = {get_temp_entity()->m_uuid, 1};
                constraint.m_wrkpl = m_wrkpl->m_uuid;
            }

            m_entities.push_back(get_temp_entity());
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
            if (m_has_tangent_head)
                m_constrain_tangent_head = !m_constrain_tangent_head;
            else
                m_constrain_tangent = !m_constrain_tangent;
        } break;

        case InToolActionID::TOGGLE_ARC: {
            if (get_temp_entity() && is_draw_contour() && m_state == State::NORMAL) {
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

        case InToolActionID::TOGGLE_BEZIER: {
            if (get_temp_entity() && is_draw_contour() && m_state == State::NORMAL) {
                if (m_temp_line) {
                    m_constrain_tangent = true;
                    m_temp_bezier = &add_entity<EntityBezier2D>();
                    m_temp_bezier->m_p1 = m_temp_line->m_p1;
                    m_temp_bezier->m_p2 = m_temp_line->m_p2;
                    m_temp_bezier->m_c1 = m_temp_bezier->m_p1;
                    m_temp_bezier->m_c2 = m_temp_bezier->m_p2;
                    m_temp_bezier->m_wrkpl = m_wrkpl->m_uuid;
                    m_temp_bezier->m_selection_invisible = true;
                    for (auto constraint : m_constraints) {
                        for (unsigned int pt = 1; pt <= 2; pt++) {
                            constraint->replace_point({m_temp_line->m_uuid, pt}, {m_temp_bezier->m_uuid, pt});
                        }
                    }
                    get_doc().m_entities.erase(m_temp_line->m_uuid);
                    m_temp_line = nullptr;
                    m_entities.back() = m_temp_bezier;
                }
                else if (m_temp_bezier) {
                    m_constrain_tangent = false;
                    m_temp_line = &add_entity<EntityLine2D>();
                    m_temp_line->m_p1 = m_temp_bezier->m_p1;
                    m_temp_line->m_p2 = m_temp_bezier->m_p2;
                    m_temp_line->m_wrkpl = m_wrkpl->m_uuid;
                    m_temp_line->m_selection_invisible = true;
                    for (auto constraint : m_constraints) {
                        for (unsigned int pt = 1; pt <= 2; pt++) {
                            constraint->replace_point({m_temp_bezier->m_uuid, pt}, {m_temp_line->m_uuid, pt});
                        }
                    }
                    get_doc().m_entities.erase(m_temp_bezier->m_uuid);
                    m_temp_bezier = nullptr;
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

std::optional<ConstraintType> ToolDrawContour::get_head_constraint(const Entity &en_head, const Entity &en_target)
{
    if (is_placing_center())
        return {};
    auto t_head = en_head.get_type();
    auto t_target = en_target.get_type();
    using ET = Entity::Type;

    if ((t_head == ET::LINE_2D && t_target == ET::ARC_2D) || (t_head == ET::ARC_2D && t_target == ET::LINE_2D))
        return Constraint::Type::ARC_LINE_TANGENT;
    else if (t_head == ET::LINE_2D && t_target == ET::LINE_2D)
        return Constraint::Type::PARALLEL;
    else if (t_head == ET::ARC_2D && t_target == ET::ARC_2D)
        return Constraint::Type::ARC_ARC_TANGENT;
    else
        return {};
}


void ToolDrawContour::update_tip()
{
    std::vector<ActionLabelInfo> actions;

    if (is_placing_center())
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

    std::vector<ConstraintType> constraint_icons;

    m_has_tangent_head = false;
    std::string constraint_tip;
    if (m_constrain) {
        std::string what = "to";
        switch (m_state) {
        case State::CENTER:
            what = "center";
            break;
        case State::BEZIER_C1:
            what = "from control";
            break;
        case State::BEZIER_C2:
            what = "to control";
            break;
        default:;
        }

        constraint_tip = get_constrain_tip(what);
        if (auto ct = get_constraint_type()) {
            constraint_icons.push_back(*ct);
            if (*ct == ConstraintType::POINTS_COINCIDENT) {
                if (auto en_a = dynamic_cast<const IEntityTangent *>(get_temp_entity())) {
                    auto enp = m_intf.get_hover_selection().value().get_entity_and_point();
                    if (auto en_t = dynamic_cast<const IEntityTangent *>(&get_entity(enp.entity))) {
                        if (en_t->is_valid_tangent_point(enp.point)) {
                            auto arc_tangent = glm::normalize(en_a->get_tangent_at_point(get_head_point()));
                            auto target_tanget = glm::normalize(en_t->get_tangent_at_point(enp.point));
                            auto angle = glm::degrees(acos(glm::dot(arc_tangent, target_tanget)));
                            if (std::abs(angle - 180) < 45)
                                m_has_tangent_head = true;
                        }
                    }
                }
            }
        }
    }

    if (m_has_tangent_head && m_constrain_tangent_head) {
        auto &en_head = *get_temp_entity();
        auto &en_target = get_entity(m_intf.get_hover_selection().value().get_entity_and_point().entity);
        const auto constraint_type = get_head_constraint(en_head, en_target);
        if (constraint_type) {
            constraint_icons.push_back(*constraint_type);
        }
    }


    if (m_has_tangent_head) {
        if (m_constrain_tangent_head)
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "head tangent constraint off");
        else
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "head tangent constraint on");
    }
    else if (m_last_tangent_point) {
        if (m_constrain_tangent)
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "tangent constraint off");
        else
            actions.emplace_back(InToolActionID::TOGGLE_TANGENT_CONSTRAINT, "tangent constraint on");
    }

    if (get_temp_entity() && is_draw_contour() && m_state == State::NORMAL) {
        auto et = get_temp_entity()->get_type();
        if (et == Entity::Type::ARC_2D) {
            actions.emplace_back(InToolActionID::TOGGLE_ARC, "arc off");
        }
        else if (et == Entity::Type::BEZIER_2D) {
            actions.emplace_back(InToolActionID::TOGGLE_BEZIER, "bezier off");
        }
        else {
            actions.emplace_back(InToolActionID::TOGGLE_ARC, "arc on");
            actions.emplace_back(InToolActionID::TOGGLE_BEZIER, "bezier on");
        }
    }

    if (m_temp_arc && !(m_constrain_tangent && m_last_tangent_point))
        actions.emplace_back(InToolActionID::FLIP_ARC);

    std::string tip;

    if (!m_tangent_valid) {
        tip += "invalid tangent ";
    }

    tip += constraint_tip;

    if (auto ct = get_auto_constraint()) {
        if (*ct == Constraint::Type::HORIZONTAL)
            tip += " horizontal";
        else if (*ct == Constraint::Type::VERTICAL)
            tip += " vertical";
        constraint_icons.push_back(*ct);
    }
    if (!tip.size())
        tip = " ";
    m_intf.tool_bar_set_tool_tip(tip);

    glm::vec3 v = {NAN, NAN, NAN};
    if (!is_placing_center()) {
        if (auto en_t = dynamic_cast<IEntityTangent *>(get_temp_entity())) {
            unsigned int tangent_point = 2;
            if (m_temp_line) {
                tangent_point = 2;
            }
            else if (m_temp_arc) {
                tangent_point = get_arc_head_point();
            }
            v = m_wrkpl->transform_relative(en_t->get_tangent_at_point(tangent_point));
        }
    }
    m_intf.set_constraint_icons(get_cursor_pos_for_workplane(*m_wrkpl), v, constraint_icons);


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
    if (m_temp_bezier)
        return m_temp_bezier;
    return nullptr;
}

} // namespace dune3d
