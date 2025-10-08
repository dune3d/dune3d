#include "system.hpp"
#include "solvespace/src/solvespace.h"
#include "solvespace/src/sketch.h"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_step.hpp"
#include "document/entity/entity_point2d.hpp"
#include "document/entity/entity_document.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_bezier3d.hpp"
#include "document/entity/entity_cluster.hpp"
#include "document/entity/entity_text.hpp"
#include "document/entity/entity_picture.hpp"
#include "document/constraint/all_constraints.hpp"
#include "document/group/group.hpp"
#include "document/group/group_extrude.hpp"
#include "document/group/group_lathe.hpp"
#include "document/group/group_revolve.hpp"
#include "document/group/group_linear_array.hpp"
#include "document/group/group_polar_array.hpp"
#include "document/group/group_mirror_hv.hpp"
#include "document/group/group_clone.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <set>
#include <iostream>

Sketch SolveSpace::SK = {};

void SolveSpace::Platform::FatalError(const std::string &message)
{
    fprintf(stderr, "%s", message.c_str());
    abort();
}

void Group::GenerateEquations(IdList<Equation, hEquation> *)
{
    // Nothing to do for now.
}

namespace dune3d {


static std::mutex s_sys_mutex;

System::System(Document &doc, const UUID &grp, const UUID &constraint_exclude)
    : m_sys(std::make_unique<SolveSpace::System>()), m_doc(doc), m_solve_group(grp), m_lock(s_sys_mutex)
{
    auto &solve_group = doc.get_group(m_solve_group);
    for (auto &[uu, constraint] : m_doc.m_constraints) {
        if (constraint->m_group == m_solve_group)
            if (auto ps = dynamic_cast<const IConstraintPreSolve *>(constraint.get()))
                ps->pre_solve(m_doc);
    }
    if (auto ps = dynamic_cast<const IGroupPreSolve *>(&solve_group)) {
        ps->pre_solve(m_doc);
    }

    std::set<Entity *> entities;
    std::set<Constraint *> constraints;
    for (const auto &[uu, entity] : m_doc.m_entities) {
        if (entity->m_group != m_solve_group)
            continue;
        entities.insert(entity.get());
        auto referenced_entities = entity->get_referenced_entities();
        for (const auto &uu : referenced_entities) {
            entities.insert(&doc.get_entity(uu));
        }
    }
    for (const auto &[uu, constraint] : m_doc.m_constraints) {
        if (constraint->m_group != m_solve_group)
            continue;
        if (uu == constraint_exclude)
            continue;
        constraints.insert(constraint.get());
        auto referenced_entities = constraint->get_referenced_entities();
        for (const auto &uu : referenced_entities) {
            entities.insert(&doc.get_entity(uu));
        }
    }
    {
        auto referenced_entities = solve_group.get_referenced_entities(doc);
        for (const auto &uu : referenced_entities) {
            entities.insert(&doc.get_entity(uu));
        }
    }
    {
        std::set<Entity *> other_entities;
        for (auto entity : entities) {
            auto referenced_entities = entity->get_referenced_entities();
            for (const auto &uu : referenced_entities) {
                other_entities.insert(&doc.get_entity(uu));
            }
        }
        entities.insert(other_entities.begin(), other_entities.end());
    }
    for (auto entity : entities) {
        entity->accept(*this);
    }
    for (auto constraint : constraints) {
        constraint->accept(*this);
    }

    switch (solve_group.get_type()) {
    case Group::Type::EXTRUDE:
        add(dynamic_cast<const GroupExtrude &>(solve_group));
        break;
    case Group::Type::LATHE:
        add(dynamic_cast<const GroupLathe &>(solve_group));
        break;
    case Group::Type::REVOLVE:
        add(dynamic_cast<const GroupRevolve &>(solve_group));
        break;
    case Group::Type::LINEAR_ARRAY:
        add(dynamic_cast<const GroupLinearArray &>(solve_group));
        break;
    case Group::Type::POLAR_ARRAY:
        add(dynamic_cast<const GroupPolarArray &>(solve_group));
        break;
    case Group::Type::MIRROR_HORIZONTAL:
    case Group::Type::MIRROR_VERTICAL:
        add(dynamic_cast<const GroupMirrorHV &>(solve_group));
        break;
    case Group::Type::CLONE:
        add(dynamic_cast<const GroupClone &>(solve_group));
        break;
    default:;
    }
}

void System::visit(const EntityLine3D &line)
{
    const auto group = get_group_index(line);

    std::array<unsigned int, 2> points;
    for (unsigned int point = 1; point <= 2; point++) {
        auto e = get_entity_ref(EntityRef{line.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = e;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(line.m_group, line.m_uuid, point, axis);
            // std::cout << line.m_name << "." << point << "." << axis << "=" << eb.param[axis].v << std::endl;
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto e = get_entity_ref(EntityRef{line.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::LINE_SEGMENT;
    eb.h.v = e;
    eb.group.v = group;
    eb.point[0].v = points.at(0);
    eb.point[1].v = points.at(1);
    SK.entity.Add(&eb);
    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityLine2D &line)
{
    const auto group = get_group_index(line);

    std::array<unsigned int, 2> points;
    for (unsigned int point = 1; point <= 2; point++) {
        auto e = get_entity_ref(EntityRef{line.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = e;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{line.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(line.m_group, line.m_uuid, point, axis);
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto e = get_entity_ref(EntityRef{line.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::LINE_SEGMENT;
    eb.h.v = e;
    eb.group.v = group;
    eb.point[0].v = points.at(0);
    eb.point[1].v = points.at(1);
    SK.entity.Add(&eb);
    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityPoint2D &epoint)
{
    const auto group = get_group_index(epoint);

    auto e = get_entity_ref(EntityRef{epoint.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::POINT_IN_2D;
    eb.h.v = e;
    eb.group.v = group;
    eb.workplane.v = get_entity_ref(EntityRef{epoint.m_wrkpl, 0});
    for (unsigned int axis = 0; axis < 2; axis++) {
        eb.param[axis].v = add_param(epoint.m_group, epoint.m_uuid, 0, axis);
    }
    SK.entity.Add(&eb);
}

void System::visit(const EntityArc2D &arc)
{
    const auto group = get_group_index(arc);

    std::array<unsigned int, 3> points;
    for (unsigned int point = 1; point <= 3; point++) {
        auto e = get_entity_ref(EntityRef{arc.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = e;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{arc.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(arc.m_group, arc.m_uuid, point, axis);
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto e = get_entity_ref(EntityRef{arc.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::ARC_OF_CIRCLE;
    eb.h.v = e;
    eb.group.v = group;
    eb.workplane.v = get_entity_ref(EntityRef{arc.m_wrkpl, 0});
    eb.normal.v = get_entity_ref(EntityRef{arc.m_wrkpl, 2});
    eb.point[0].v = points.at(2); // center
    eb.point[1].v = points.at(0); // from
    eb.point[2].v = points.at(1); // to
    eb.noEquation = arc.m_no_radius_constraint;
    SK.entity.Add(&eb);
    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityArc3D &arc)
{
    const auto group = get_group_index(arc);

    std::array<unsigned int, 3> points;
    for (unsigned int point = 1; point <= 3; point++) {
        auto e = get_entity_ref(EntityRef{arc.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = e;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(arc.m_group, arc.m_uuid, point, axis);
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto en_normal = get_entity_ref(EntityRef{arc.m_uuid, 4});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::NORMAL_IN_3D;
        eb.h.v = en_normal;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 4; axis++) {
            eb.param[axis].v = add_param(arc.m_group, arc.m_uuid, 4, (axis + 3) % 4);
        }
        SK.entity.Add(&eb);
    }

    auto e = get_entity_ref(EntityRef{arc.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::ARC_OF_CIRCLE;
    eb.h.v = e;
    eb.group.v = group;
    eb.normal.v = en_normal;
    eb.point[0].v = points.at(2);
    eb.point[1].v = points.at(0);
    eb.point[2].v = points.at(1);
    SK.entity.Add(&eb);

    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityCircle2D &circle)
{
    const auto group = get_group_index(circle);

    auto epoint = get_entity_ref(EntityRef{circle.m_uuid, 1});

    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = epoint;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{circle.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(circle.m_group, circle.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto edistance = get_entity_ref(EntityRef{circle.m_uuid, 2});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::DISTANCE;
        eb.h.v = edistance;
        eb.group.v = group;
        eb.param[0].v = add_param(circle.m_group, circle.m_uuid, 0, 0);
        SK.entity.Add(&eb);
    }

    auto e = get_entity_ref(EntityRef{circle.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::CIRCLE;
    eb.h.v = e;
    eb.group.v = group;
    eb.normal.v = get_entity_ref(EntityRef{circle.m_wrkpl, 2});
    eb.distance.v = edistance;
    eb.point[0].v = epoint;
    SK.entity.Add(&eb);


    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}
void System::visit(const EntityCircle3D &circle)
{
    const auto group = get_group_index(circle);

    auto epoint = get_entity_ref(EntityRef{circle.m_uuid, 1});

    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = epoint;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(circle.m_group, circle.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto edistance = get_entity_ref(EntityRef{circle.m_uuid, 2});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::DISTANCE;
        eb.h.v = edistance;
        eb.group.v = group;
        eb.param[0].v = add_param(circle.m_group, circle.m_uuid, 0, 0);
        SK.entity.Add(&eb);
    }

    auto en_normal = get_entity_ref(EntityRef{circle.m_uuid, 3});
    {
        EntityBase eb = {};
        eb.noEquation = true;
        eb.type = EntityBase::Type::NORMAL_IN_3D;
        eb.h.v = en_normal;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 4; axis++) {
            eb.param[axis].v = add_param(circle.m_group, circle.m_uuid, 2, (axis + 3) % 4);
        }
        SK.entity.Add(&eb);
    }

    auto e = get_entity_ref(EntityRef{circle.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::CIRCLE;
    eb.h.v = e;
    eb.group.v = group;
    eb.normal.v = en_normal;
    eb.distance.v = edistance;
    eb.point[0].v = epoint;
    SK.entity.Add(&eb);
}

void System::visit(const EntityWorkplane &wrkpl)
{
    const auto group = get_group_index(wrkpl);

    auto en_origin = get_entity_ref(EntityRef{wrkpl.m_uuid, 1});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = en_origin;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(wrkpl.m_group, wrkpl.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto en_normal = get_entity_ref(EntityRef{wrkpl.m_uuid, 2});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::NORMAL_IN_3D;
        eb.h.v = en_normal;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 4; axis++) {
            eb.param[axis].v = add_param(wrkpl.m_group, wrkpl.m_uuid, 2, (axis + 3) % 4);
        }
        SK.entity.Add(&eb);
    }

    auto en_wrkpl = get_entity_ref(EntityRef{wrkpl.m_uuid, 0});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::WORKPLANE;
        eb.h.v = en_wrkpl;
        eb.group.v = group;
        eb.point[0].v = en_origin;
        eb.normal.v = en_normal;
        SK.entity.Add(&eb);
    }
}

static void AddEq(IdList<Equation, hEquation> *l, Expr *expr)
{
    Equation eq;
    eq.e = expr;
    l->AddAndAssignId(&eq);
}

void System::visit(const EntitySTEP &step)
{
    const auto group = get_group_index(step);

    auto en_origin = get_entity_ref(EntityRef{step.m_uuid, 1});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = en_origin;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(step.m_group, step.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }


    auto en_normal = get_entity_ref(EntityRef{step.m_uuid, 2});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::NORMAL_IN_3D;
        eb.h.v = en_normal;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 4; axis++) {
            eb.param[axis].v = add_param(step.m_group, step.m_uuid, 2, (axis + 3) % 4);
        }
        SK.entity.Add(&eb);
    }


    for (auto &[idx, p] : step.m_anchors) {
        auto en_p = get_entity_ref(EntityRef{step.m_uuid, idx});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = en_p;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(step.m_group, step.m_uuid, idx, axis);
        }
        SK.entity.Add(&eb);
        if (step.m_group == m_solve_group) {
            auto eb_origin = SK.GetEntity({en_origin});
            auto eb_normal = SK.GetEntity({en_normal});
            auto ex = eb_normal->NormalGetExprs()
                              .Rotate(ExprVector::From(p.x, p.y, p.z))
                              .Plus(eb_origin->PointGetExprs())
                              .Minus(eb.PointGetExprs());

            AddEq(&m_sys->eq, ex.x);
            AddEq(&m_sys->eq, ex.y);
            AddEq(&m_sys->eq, ex.z);
        }
    }
}

void System::visit(const EntityDocument &en_doc)
{
    const auto group = get_group_index(en_doc);

    auto en_origin = get_entity_ref(EntityRef{en_doc.m_uuid, 1});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = en_origin;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(en_doc.m_group, en_doc.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }


    auto en_normal = get_entity_ref(EntityRef{en_doc.m_uuid, 2});
    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::NORMAL_IN_3D;
        eb.h.v = en_normal;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 4; axis++) {
            eb.param[axis].v = add_param(en_doc.m_group, en_doc.m_uuid, 2, (axis + 3) % 4);
        }
        SK.entity.Add(&eb);
    }
}

void System::visit(const EntityBezier2D &bezier)
{
    const auto group = get_group_index(bezier);

    std::array<unsigned int, 4> points;
    for (unsigned int point = 1; point <= 4; point++) {
        auto e = get_entity_ref(EntityRef{bezier.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = e;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{bezier.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(bezier.m_group, bezier.m_uuid, point, axis);
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto e = get_entity_ref(EntityRef{bezier.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::CUBIC;
    eb.h.v = e;
    eb.group.v = group;
    eb.point[0].v = points.at(0);
    eb.point[1].v = points.at(2);
    eb.point[2].v = points.at(3);
    eb.point[3].v = points.at(1);
    SK.entity.Add(&eb);
    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityBezier3D &bezier)
{
    const auto group = get_group_index(bezier);

    std::array<unsigned int, 4> points;
    for (unsigned int point = 1; point <= 4; point++) {
        auto e = get_entity_ref(EntityRef{bezier.m_uuid, point});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_3D;
        eb.h.v = e;
        eb.group.v = group;
        for (unsigned int axis = 0; axis < 3; axis++) {
            eb.param[axis].v = add_param(bezier.m_group, bezier.m_uuid, point, axis);
        }
        SK.entity.Add(&eb);
        points.at(point - 1) = e;
    }
    auto e = get_entity_ref(EntityRef{bezier.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::CUBIC;
    eb.h.v = e;
    eb.group.v = group;
    eb.point[0].v = points.at(0);
    eb.point[1].v = points.at(2);
    eb.point[2].v = points.at(3);
    eb.point[3].v = points.at(1);
    SK.entity.Add(&eb);
    // m_sys->entity.push_back(Slvs_MakeLineSegment(e, group, SLVS_FREE_IN_3D, points.at(0), points.at(1)));
}

void System::visit(const EntityCluster &en_cluster)
{
    const auto group = get_group_index(en_cluster);
    auto en_origin = get_entity_ref(EntityRef{en_cluster.m_uuid, 1});

    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_origin;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_cluster.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_cluster.m_group, en_cluster.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto param_scale_x = add_param(en_cluster.m_group, en_cluster.m_uuid, 2, 0);
    auto param_scale_y = add_param(en_cluster.m_group, en_cluster.m_uuid, 2, 1);
    auto param_angle = add_param(en_cluster.m_group, en_cluster.m_uuid, 3, 0);

    if (en_cluster.m_group == m_solve_group) {
        if (en_cluster.m_lock_aspect_ratio)
            AddEq(&m_sys->eq, Expr::From(hParam{param_scale_x})->Minus(Expr::From(hParam{param_scale_y})));

        if (en_cluster.m_lock_scale_x)
            AddEq(&m_sys->eq, Expr::From(hParam{param_scale_x})->Minus(Expr::From(en_cluster.m_scale_x)));

        if (en_cluster.m_lock_scale_y)
            AddEq(&m_sys->eq, Expr::From(hParam{param_scale_y})->Minus(Expr::From(en_cluster.m_scale_y)));

        if (en_cluster.m_lock_angle)
            AddEq(&m_sys->eq, Expr::From(hParam{param_angle})->Minus(Expr::From(glm::radians(en_cluster.m_angle))));
    }

    auto exangle = Expr::From({hParam{param_angle}});
    auto exsin = exangle->Sin();
    auto excos = exangle->Cos();
    for (auto &[idx, enp] : en_cluster.m_anchors) {
        auto en_p = get_entity_ref(EntityRef{en_cluster.m_uuid, idx});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_p;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_cluster.m_wrkpl, 0});

        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_cluster.m_group, en_cluster.m_uuid, idx, axis);
        }
        SK.entity.Add(&eb);

        if (en_cluster.m_group == m_solve_group) {
            auto eb_origin = SK.GetEntity({en_origin});
            auto p = en_cluster.get_anchor_point(enp);
            auto p_scaled = ExprVector::From(Expr::From(p.x)->Times(Expr::From(hParam{param_scale_x})),
                                             Expr::From(p.y)->Times(Expr::From(hParam{param_scale_y})), Expr::From(0));

            auto prx = p_scaled.x->Times(excos)->Minus(p_scaled.y->Times(exsin));
            auto pry = p_scaled.x->Times(exsin)->Plus(p_scaled.y->Times(excos));
            auto p_scaled_rot = ExprVector::From(prx, pry, Expr::From(0));
            auto ex = eb.PointGetExprsInWorkplane(eb.workplane)
                              .Minus(eb_origin->PointGetExprsInWorkplane(eb.workplane).Plus(p_scaled_rot));

            AddEq(&m_sys->eq, ex.x);
            AddEq(&m_sys->eq, ex.y);
        }
    }
}


void System::visit(const EntityText &en_text)
{
    const auto group = get_group_index(en_text);
    auto en_origin = get_entity_ref(EntityRef{en_text.m_uuid, 1});

    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_origin;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_text.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_text.m_group, en_text.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto param_scale = add_param(en_text.m_group, en_text.m_uuid, 2, 0);
    auto param_angle = add_param(en_text.m_group, en_text.m_uuid, 3, 0);

    if (en_text.m_group == m_solve_group) {
        if (en_text.m_lock_scale)
            AddEq(&m_sys->eq, Expr::From(hParam{param_scale})->Minus(Expr::From(en_text.m_scale)));

        if (en_text.m_lock_angle)
            AddEq(&m_sys->eq, Expr::From(hParam{param_angle})->Minus(Expr::From(glm::radians(en_text.m_angle))));
    }

    auto exangle = Expr::From({hParam{param_angle}});
    auto exsin = exangle->Sin();
    auto excos = exangle->Cos();

    for (auto &[idx, p] : en_text.m_anchors) {
        auto en_p = get_entity_ref(EntityRef{en_text.m_uuid, idx});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_p;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_text.m_wrkpl, 0});

        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_text.m_group, en_text.m_uuid, idx, axis);
        }
        SK.entity.Add(&eb);
        if (en_text.m_group == m_solve_group) {
            auto eb_origin = SK.GetEntity({en_origin});
            auto p_scaled = ExprVector::From(Expr::From(p.x)->Times(Expr::From(hParam{param_scale})),
                                             Expr::From(p.y)->Times(Expr::From(hParam{param_scale})), Expr::From(0));

            auto prx = p_scaled.x->Times(excos)->Minus(p_scaled.y->Times(exsin));
            auto pry = p_scaled.x->Times(exsin)->Plus(p_scaled.y->Times(excos));
            auto p_scaled_rot = ExprVector::From(prx, pry, Expr::From(0));
            auto ex = eb.PointGetExprsInWorkplane(eb.workplane)
                              .Minus(eb_origin->PointGetExprsInWorkplane(eb.workplane).Plus(p_scaled_rot));

            AddEq(&m_sys->eq, ex.x);
            AddEq(&m_sys->eq, ex.y);
        }
    }
}

void System::visit(const EntityPicture &en_pic)
{
    const auto group = get_group_index(en_pic);
    auto en_origin = get_entity_ref(EntityRef{en_pic.m_uuid, 1});

    {
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_origin;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_pic.m_wrkpl, 0});
        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_pic.m_group, en_pic.m_uuid, 1, axis);
        }
        SK.entity.Add(&eb);
    }

    auto param_scale_x = add_param(en_pic.m_group, en_pic.m_uuid, 2, 0);
    auto param_scale_y = add_param(en_pic.m_group, en_pic.m_uuid, 2, 1);
    auto param_angle = add_param(en_pic.m_group, en_pic.m_uuid, 3, 0);

    if (en_pic.m_group == m_solve_group) {
        if (en_pic.m_lock_aspect_ratio)
            AddEq(&m_sys->eq, Expr::From(hParam{param_scale_x})->Minus(Expr::From(hParam{param_scale_y})));

        if (en_pic.m_lock_angle)
            AddEq(&m_sys->eq, Expr::From(hParam{param_angle})->Minus(Expr::From(glm::radians(en_pic.m_angle))));
    }

    auto exangle = Expr::From({hParam{param_angle}});
    auto exsin = exangle->Sin();
    auto excos = exangle->Cos();
    for (auto &[idx, px_pos] : en_pic.m_anchors) {
        auto en_p = get_entity_ref(EntityRef{en_pic.m_uuid, idx});
        EntityBase eb = {};
        eb.type = EntityBase::Type::POINT_IN_2D;
        eb.h.v = en_p;
        eb.group.v = group;
        eb.workplane.v = get_entity_ref(EntityRef{en_pic.m_wrkpl, 0});

        for (unsigned int axis = 0; axis < 2; axis++) {
            eb.param[axis].v = add_param(en_pic.m_group, en_pic.m_uuid, idx, axis);
        }
        SK.entity.Add(&eb);

        if (en_pic.m_group == m_solve_group) {
            auto eb_origin = SK.GetEntity({en_origin});
            auto p_scaled = ExprVector::From(
                    Expr::From(px_pos.x - en_pic.m_width / 2)->Times(Expr::From(hParam{param_scale_x})),
                    Expr::From(px_pos.y - en_pic.m_height / 2)->Times(Expr::From(hParam{param_scale_y})),
                    Expr::From(0));

            auto prx = p_scaled.x->Times(excos)->Minus(p_scaled.y->Times(exsin));
            auto pry = p_scaled.x->Times(exsin)->Plus(p_scaled.y->Times(excos));
            auto p_scaled_rot = ExprVector::From(prx, pry, Expr::From(0));
            auto ex = eb.PointGetExprsInWorkplane(eb.workplane)
                              .Minus(eb_origin->PointGetExprsInWorkplane(eb.workplane).Plus(p_scaled_rot));

            AddEq(&m_sys->eq, ex.x);
            AddEq(&m_sys->eq, ex.y);
        }
    }
}


static void AddEq(hGroup h, IdList<Equation, hEquation> *l, Expr *expr, int index)
{
    Equation eq;
    eq.e = expr;
    eq.h = h.equation(index);
    l->Add(&eq);
}


void System::add(const GroupExtrude &group)
{

    auto dx = add_param(group.m_uuid, group.m_dvec.x);
    auto dy = add_param(group.m_uuid, group.m_dvec.y);
    auto dz = add_param(group.m_uuid, group.m_dvec.z);
    m_param_refs.emplace(dx, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 0});
    m_param_refs.emplace(dy, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 1});
    m_param_refs.emplace(dz, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 2});
    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;
    {
        ExprVector direction = ExprVector::From(hParam{dx}, hParam{dy}, hParam{dz});

        if (group.m_direction == GroupExtrude::Direction::NORMAL) {
            auto &wrkpl = m_doc.get_entity<EntityWorkplane>(group.m_wrkpl);
            auto dl = add_param(group.m_uuid, glm::dot(group.m_dvec, wrkpl.get_normal_vector()));
            auto en_wrkpl_normal = get_entity_ref(EntityRef{group.m_wrkpl, 2});
            EntityBase *a = SK.GetEntity({en_wrkpl_normal});
            ExprVector plane_normal = a->NormalExprsN();
            auto pl = Expr::From(hParam{dl});
            AddEq(hg, &m_sys->eq, direction.x->Minus(plane_normal.x->Times(pl)), eqi++);
            AddEq(hg, &m_sys->eq, direction.y->Minus(plane_normal.y->Times(pl)), eqi++);
            AddEq(hg, &m_sys->eq, direction.z->Minus(plane_normal.z->Times(pl)), eqi++);
        }
    }
    for (const auto side : {GroupExtrude::Side::TOP, GroupExtrude::Side::BOTTOM}) {
        if (!group.has_side(side))
            continue;

        ExprVector direction = ExprVector::From(hParam{dx}, hParam{dy}, hParam{dz});

        if (side == GroupExtrude::Side::BOTTOM) {
            if (group.m_mode == GroupExtrude::Mode::OFFSET_SYMMETRIC) {
                direction = direction.ScaledBy(Expr::From(-1));
            }
            else if (group.m_mode == GroupExtrude::Mode::OFFSET) {
                auto dm = add_param(group.m_uuid, group.m_offset_mul);
                m_param_refs.emplace(dm, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 0});
                direction = direction.ScaledBy(Expr::From(hParam{dm}));
            }
        }

        {
            auto en_wrkpl_origin = get_entity_ref(EntityRef{group.m_wrkpl, 1});
            auto en_leader_p1 = get_entity_ref(EntityRef{group.get_leader_line_uuid(side), 1});
            auto en_leader_p2 = get_entity_ref(EntityRef{group.get_leader_line_uuid(side), 2});
            EntityBase *a = SK.GetEntity({en_wrkpl_origin});
            EntityBase *ep1 = SK.GetEntity({en_leader_p1});
            EntityBase *ep2 = SK.GetEntity({en_leader_p2});
            ExprVector pa = a->PointGetExprs();
            ExprVector exp1 = ep1->PointGetExprs();
            ExprVector exp2 = ep2->PointGetExprs();

            AddEq(hg, &m_sys->eq, pa.x->Minus(exp1.x), eqi++);
            AddEq(hg, &m_sys->eq, pa.y->Minus(exp1.y), eqi++);
            AddEq(hg, &m_sys->eq, pa.z->Minus(exp1.z), eqi++);

            AddEq(hg, &m_sys->eq, exp2.x->Minus(exp1.x->Plus(direction.x)), eqi++);
            AddEq(hg, &m_sys->eq, exp2.y->Minus(exp1.y->Plus(direction.y)), eqi++);
            AddEq(hg, &m_sys->eq, exp2.z->Minus(exp1.z->Plus(direction.z)), eqi++);
        }

        for (const auto &[uu, it] : m_doc.m_entities) {
            if (it->m_group != group.m_source_group)
                continue;
            if (it->m_construction)
                continue;
            if (it->get_type() == Entity::Type::LINE_2D) {
                const auto &li = dynamic_cast<const EntityLine2D &>(*it);
                if (li.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_line_uu = group.get_entity_uuid(side, uu);

                {

                    for (unsigned int pt = 1; pt <= 2; pt++) {
                        auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                        auto en_new_p = get_entity_ref(EntityRef{new_line_uu, pt});
                        EntityBase *eorig = SK.GetEntity({en_orig_p});
                        EntityBase *enew = SK.GetEntity({en_new_p});
                        ExprVector exorig = eorig->PointGetExprs();
                        ExprVector exnew = enew->PointGetExprs();
                        AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(direction.x)), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(direction.y)), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z->Plus(direction.z)), eqi++);
                    }
                }

                for (unsigned int pt = 1; pt <= 2; pt++) {

                    auto ex_line_uu = group.get_extrusion_line_uuid(side, uu, pt);
                    auto ev_orig_p = SK.GetEntity({get_entity_ref(EntityRef{uu, pt})})->PointGetExprs();
                    auto ev_new_p = SK.GetEntity({get_entity_ref(EntityRef{new_line_uu, pt})})->PointGetExprs();

                    auto ev_ex_1 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 1})})->PointGetExprs();
                    auto ev_ex_2 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 2})})->PointGetExprs();


                    AddEq(hg, &m_sys->eq, ev_orig_p.x->Minus(ev_ex_1.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.y->Minus(ev_ex_1.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.z->Minus(ev_ex_1.z), eqi++);

                    AddEq(hg, &m_sys->eq, ev_new_p.x->Minus(ev_ex_2.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.y->Minus(ev_ex_2.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.z->Minus(ev_ex_2.z), eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_2D) {
                const auto &bez = dynamic_cast<const EntityBezier2D &>(*it);
                if (bez.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_bez_uu = group.get_entity_uuid(side, uu);

                {
                    for (unsigned int pt = 1; pt <= 4; pt++) {
                        auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                        auto en_new_p = get_entity_ref(EntityRef{new_bez_uu, pt});
                        EntityBase *eorig = SK.GetEntity({en_orig_p});
                        EntityBase *enew = SK.GetEntity({en_new_p});
                        ExprVector exorig = eorig->PointGetExprs();
                        ExprVector exnew = enew->PointGetExprs();
                        AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(direction.x)), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(direction.y)), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z->Plus(direction.z)), eqi++);
                    }
                }

                for (unsigned int pt = 1; pt <= 2; pt++) {

                    auto ex_line_uu = group.get_extrusion_line_uuid(side, uu, pt);
                    auto ev_orig_p = SK.GetEntity({get_entity_ref(EntityRef{uu, pt})})->PointGetExprs();
                    auto ev_new_p = SK.GetEntity({get_entity_ref(EntityRef{new_bez_uu, pt})})->PointGetExprs();

                    auto ev_ex_1 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 1})})->PointGetExprs();
                    auto ev_ex_2 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 2})})->PointGetExprs();


                    AddEq(hg, &m_sys->eq, ev_orig_p.x->Minus(ev_ex_1.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.y->Minus(ev_ex_1.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.z->Minus(ev_ex_1.z), eqi++);

                    AddEq(hg, &m_sys->eq, ev_new_p.x->Minus(ev_ex_2.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.y->Minus(ev_ex_2.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.z->Minus(ev_ex_2.z), eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::ARC_2D) {
                const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
                if (arc.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_arc_uu = group.get_entity_uuid(side, uu);

                for (unsigned int pt = 1; pt <= 3; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_arc_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(direction.x)), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(direction.y)), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z->Plus(direction.z)), eqi++);
                }
                {
                    auto en_orig_n = get_entity_ref(EntityRef{arc.m_wrkpl, 2});
                    auto en_new_n = get_entity_ref(EntityRef{new_arc_uu, 4});
                    EntityBase *eorig = SK.GetEntity({en_orig_n});
                    EntityBase *enew = SK.GetEntity({en_new_n});
                    enew->noEquation = true;
                    auto eqo = eorig->NormalGetExprs();
                    auto eqn = enew->NormalGetExprs();
                    AddEq(hg, &m_sys->eq, eqo.vx->Minus(eqn.vx), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vy->Minus(eqn.vy), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vz->Minus(eqn.vz), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.w->Minus(eqn.w), eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_2D) {
                const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
                if (circle.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_circle_uu = group.get_entity_uuid(side, uu);

                {
                    unsigned int pt = 1;
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(direction.x)), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(direction.y)), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z->Plus(direction.z)), eqi++);
                }

                {
                    unsigned int pt = 1;

                    auto ex_line_uu = group.get_extrusion_line_uuid(side, uu, pt);
                    auto ev_orig_p = SK.GetEntity({get_entity_ref(EntityRef{uu, pt})})->PointGetExprs();
                    auto ev_new_p = SK.GetEntity({get_entity_ref(EntityRef{new_circle_uu, pt})})->PointGetExprs();

                    auto ev_ex_1 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 1})})->PointGetExprs();
                    auto ev_ex_2 = SK.GetEntity({get_entity_ref(EntityRef{ex_line_uu, 2})})->PointGetExprs();


                    AddEq(hg, &m_sys->eq, ev_orig_p.x->Minus(ev_ex_1.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.y->Minus(ev_ex_1.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_orig_p.z->Minus(ev_ex_1.z), eqi++);

                    AddEq(hg, &m_sys->eq, ev_new_p.x->Minus(ev_ex_2.x), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.y->Minus(ev_ex_2.y), eqi++);
                    AddEq(hg, &m_sys->eq, ev_new_p.z->Minus(ev_ex_2.z), eqi++);
                }

                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 0});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 0});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    AddEq(hg, &m_sys->eq, eorig->CircleGetRadiusExpr()->Minus(enew->CircleGetRadiusExpr()), eqi++);
                }

                {
                    auto en_orig_n = get_entity_ref(EntityRef{circle.m_wrkpl, 2});
                    auto en_new_n = get_entity_ref(EntityRef{new_circle_uu, 3});
                    EntityBase *eorig = SK.GetEntity({en_orig_n});
                    EntityBase *enew = SK.GetEntity({en_new_n});
                    enew->noEquation = true;
                    auto eqo = eorig->NormalGetExprs();
                    auto eqn = enew->NormalGetExprs();
                    AddEq(hg, &m_sys->eq, eqo.vx->Minus(eqn.vx), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vy->Minus(eqn.vy), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vz->Minus(eqn.vz), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.w->Minus(eqn.w), eqi++);
                }
            }
        }
    }
}

void System::add(const GroupLathe &group)
{
    unsigned int eqi = 0;
    const auto hg = hGroup{(uint32_t)group.get_index() + 1};

    for (const auto &[uu, en] : m_doc.m_entities) {
        if (en->m_group != m_solve_group)
            continue;
        if (en->m_kind != ItemKind::GENRERATED)
            continue;
        if (en->get_type() != Entity::Type::CIRCLE_3D)
            continue;
        {
            auto en_origin = SK.GetEntity({get_entity_ref({uu, 1})});
            auto origin = en_origin->PointGetNum();
            auto ex = en_origin->PointGetExprs();
            AddEq(hg, &m_sys->eq, ex.x->Minus(Expr::From(origin.x)), eqi++);
            AddEq(hg, &m_sys->eq, ex.y->Minus(Expr::From(origin.y)), eqi++);
            AddEq(hg, &m_sys->eq, ex.z->Minus(Expr::From(origin.z)), eqi++);
        }
        {
            auto en_dist = SK.GetEntity({get_entity_ref({uu, 2})});
            auto dist = en_dist->DistanceGetNum();
            auto ex = en_dist->DistanceGetExpr();
            AddEq(hg, &m_sys->eq, ex->Minus(Expr::From(dist)), eqi++);
        }
        {
            auto en_normal = SK.GetEntity({get_entity_ref({uu, 3})});
            en_normal->noEquation = true;
            auto normal = en_normal->NormalGetNum();
            auto ex = en_normal->NormalGetExprs();
            AddEq(hg, &m_sys->eq, ex.vx->Minus(Expr::From(normal.vx)), eqi++);
            AddEq(hg, &m_sys->eq, ex.vy->Minus(Expr::From(normal.vy)), eqi++);
            AddEq(hg, &m_sys->eq, ex.vz->Minus(Expr::From(normal.vz)), eqi++);
            AddEq(hg, &m_sys->eq, ex.w->Minus(Expr::From(normal.w)), eqi++);
        }
    }
}


static ExprQuaternion quat_from_axis_angle(ExprVector axis, Expr *dtheta)
{
    ExprQuaternion q;
    auto c = dtheta->Times(Expr::From(0.5))->Cos();
    auto s = dtheta->Times(Expr::From(0.5))->Sin();
    axis = axis.WithMagnitude(s);
    q.w = c;
    q.vx = axis.x;
    q.vy = axis.y;
    q.vz = axis.z;
    return q;
}

void System::add(const GroupRevolve &group)
{

    auto angle_param = add_param(group.m_uuid, glm::radians(group.m_angle));
    m_param_refs.emplace(angle_param, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 0});
    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;
    const auto org = m_doc.get_point(group.m_origin);
    auto origin = ExprVector::From(org.x, org.y, org.z);
    const auto axv = group.get_direction(m_doc).value();

    for (const auto side : {GroupRevolve::Side::TOP, GroupRevolve::Side::BOTTOM}) {
        if (!group.has_side(side))
            continue;

        auto angle = Expr::From(hParam{angle_param});
        if (side == GroupRevolve::Side::BOTTOM) {
            if (group.m_mode == GroupRevolve::Mode::OFFSET_SYMMETRIC) {
                angle = angle->Times(Expr::From(-1));
            }
            else if (group.m_mode == GroupRevolve::Mode::OFFSET) {
                auto dm = add_param(group.m_uuid, group.m_offset_mul);
                m_param_refs.emplace(dm, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 0});
                angle = angle->Times(Expr::From(hParam{dm}));
            }
        }

        auto quat = quat_from_axis_angle(ExprVector::From(axv.x, axv.y, axv.z), angle);


        for (const auto &[uu, it] : m_doc.m_entities) {
            if (it->m_group != group.m_source_group)
                continue;
            if (it->m_construction)
                continue;
            if (it->get_type() == Entity::Type::LINE_2D) {
                const auto &li = dynamic_cast<const EntityLine2D &>(*it);
                if (li.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_line_uu = group.get_entity_uuid(side, uu);


                for (unsigned int pt = 1; pt <= 2; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_line_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    auto rot = quat.Rotate(exorig.Minus(origin)).Plus(origin);
                    auto d = rot.Minus(exnew);

                    AddEq(hg, &m_sys->eq, d.x, eqi++);
                    AddEq(hg, &m_sys->eq, d.y, eqi++);
                    AddEq(hg, &m_sys->eq, d.z, eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_2D) {
                const auto &bez = dynamic_cast<const EntityBezier2D &>(*it);
                if (bez.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_bez_uu = group.get_entity_uuid(side, uu);

                for (unsigned int pt = 1; pt <= 4; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_bez_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    auto rot = quat.Rotate(exorig.Minus(origin)).Plus(origin);
                    auto d = rot.Minus(exnew);

                    AddEq(hg, &m_sys->eq, d.x, eqi++);
                    AddEq(hg, &m_sys->eq, d.y, eqi++);
                    AddEq(hg, &m_sys->eq, d.z, eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_2D) {
                const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
                if (circle.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_circle_uu = group.get_entity_uuid(side, uu);

                {
                    unsigned int pt = 1;
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();

                    auto rot = quat.Rotate(exorig.Minus(origin)).Plus(origin);
                    auto d = rot.Minus(exnew);

                    AddEq(hg, &m_sys->eq, d.x, eqi++);
                    AddEq(hg, &m_sys->eq, d.y, eqi++);
                    AddEq(hg, &m_sys->eq, d.z, eqi++);
                }


                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 0});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 0});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    AddEq(hg, &m_sys->eq, eorig->CircleGetRadiusExpr()->Minus(enew->CircleGetRadiusExpr()), eqi++);
                }

                {
                    auto en_orig_n = get_entity_ref(EntityRef{circle.m_wrkpl, 2});
                    auto en_new_n = get_entity_ref(EntityRef{new_circle_uu, 3});
                    EntityBase *eorig = SK.GetEntity({en_orig_n});
                    EntityBase *enew = SK.GetEntity({en_new_n});
                    enew->noEquation = true;
                    auto eqo = quat.Times(eorig->NormalGetExprs());
                    auto eqn = enew->NormalGetExprs();
                    AddEq(hg, &m_sys->eq, eqo.vx->Minus(eqn.vx), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vy->Minus(eqn.vy), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vz->Minus(eqn.vz), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.w->Minus(eqn.w), eqi++);
                }
            }

            else if (it->get_type() == Entity::Type::ARC_2D) {
                const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
                if (arc.m_wrkpl != group.m_wrkpl)
                    continue;
                auto new_arc_uu = group.get_entity_uuid(side, uu);

                for (unsigned int pt = 1; pt <= 3; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_arc_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();

                    auto rot = quat.Rotate(exorig.Minus(origin)).Plus(origin);
                    auto d = rot.Minus(exnew);

                    AddEq(hg, &m_sys->eq, d.x, eqi++);
                    AddEq(hg, &m_sys->eq, d.y, eqi++);
                    AddEq(hg, &m_sys->eq, d.z, eqi++);
                }
                {
                    auto en_orig_n = get_entity_ref(EntityRef{arc.m_wrkpl, 2});
                    auto en_new_n = get_entity_ref(EntityRef{new_arc_uu, 4});
                    EntityBase *eorig = SK.GetEntity({en_orig_n});
                    EntityBase *enew = SK.GetEntity({en_new_n});
                    enew->noEquation = true;
                    auto eqo = quat.Times(eorig->NormalGetExprs());
                    auto eqn = enew->NormalGetExprs();
                    AddEq(hg, &m_sys->eq, eqo.vx->Minus(eqn.vx), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vy->Minus(eqn.vy), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vz->Minus(eqn.vz), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.w->Minus(eqn.w), eqi++);
                }
            }
        }
    }
}

void System::add_replicate(const GroupReplicate &group, CreateEq create_eq2, CreateEq create_eq3, CreateEqN create_eq_n,
                           unsigned int &eqi)
{
    auto hg = hGroup{(uint32_t)group.get_index() + 1};

    for (const auto &[uu, it] : m_doc.m_entities) {
        if (!group.is_source_group(m_doc, it->m_group))
            continue;
        if (it->m_construction)
            continue;
        for (unsigned int instance = 0; instance < group.get_count(); instance++) {
            if (it->get_type() == Entity::Type::LINE_2D) {
                const auto &li = dynamic_cast<const EntityLine2D &>(*it);
                if (li.m_wrkpl != group.m_active_wrkpl)
                    continue;
                auto new_line_uu = group.get_entity_uuid(uu, instance);
                auto en_wrkpl = hEntity{get_entity_ref(EntityRef{li.m_wrkpl, 0})};

                for (unsigned int pt = 1; pt <= 2; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_line_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprsInWorkplane(en_wrkpl);
                    ExprVector exnew = enew->PointGetExprsInWorkplane(en_wrkpl);
                    create_eq2(exorig, exnew, instance);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_2D) {
                const auto &li = dynamic_cast<const EntityBezier2D &>(*it);
                if (li.m_wrkpl != group.m_active_wrkpl)
                    continue;
                auto new_bez_uu = group.get_entity_uuid(uu, instance);
                auto en_wrkpl = hEntity{get_entity_ref(EntityRef{li.m_wrkpl, 0})};

                for (unsigned int pt = 1; pt <= 4; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_bez_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprsInWorkplane(en_wrkpl);
                    ExprVector exnew = enew->PointGetExprsInWorkplane(en_wrkpl);
                    create_eq2(exorig, exnew, instance);
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_2D) {
                const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
                if (circle.m_wrkpl != group.m_active_wrkpl)
                    continue;
                auto new_circle_uu = group.get_entity_uuid(uu, instance);
                auto en_wrkpl = hEntity{get_entity_ref(EntityRef{circle.m_wrkpl, 0})};


                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 1});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 1});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprsInWorkplane(en_wrkpl);
                    ExprVector exnew = enew->PointGetExprsInWorkplane(en_wrkpl);
                    create_eq2(exorig, exnew, instance);
                }
                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 0});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 0});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    AddEq(hg, &m_sys->eq, eorig->CircleGetRadiusExpr()->Minus(enew->CircleGetRadiusExpr()), eqi++);
                }
            }
            else if (it->get_type() == Entity::Type::ARC_2D) {
                const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
                if (arc.m_wrkpl != group.m_active_wrkpl)
                    continue;
                auto new_arc_uu = group.get_entity_uuid(uu, instance);
                auto en_wrkpl = hEntity{get_entity_ref(EntityRef{arc.m_wrkpl, 0})};

                for (unsigned int pt = 1; pt <= 3; pt++) {
                    unsigned int pt_orig = pt;
                    if (group.get_mirror_arc(instance)) {
                        if (pt_orig == 1)
                            pt_orig = 2;
                        else if (pt_orig == 2)
                            pt_orig = 1;
                    }
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt_orig});
                    auto en_new_p = get_entity_ref(EntityRef{new_arc_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprsInWorkplane(en_wrkpl);
                    ExprVector exnew = enew->PointGetExprsInWorkplane(en_wrkpl);
                    create_eq2(exorig, exnew, instance);
                }
            }
            else if (it->get_type() == Entity::Type::LINE_3D) {
                auto new_line_uu = group.get_entity_uuid(uu, instance);

                for (unsigned int pt = 1; pt <= 2; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_line_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    create_eq3(exorig, exnew, instance);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_3D) {
                auto new_bez_uu = group.get_entity_uuid(uu, instance);

                for (unsigned int pt = 1; pt <= 4; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_bez_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    create_eq3(exorig, exnew, instance);
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_3D) {
                auto new_circle_uu = group.get_entity_uuid(uu, instance);
                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 1});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 1});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    create_eq3(exorig, exnew, instance);
                }
                {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, 0});
                    auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 0});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    AddEq(hg, &m_sys->eq, eorig->CircleGetRadiusExpr()->Minus(enew->CircleGetRadiusExpr()), eqi++);
                }
                {
                    auto en_orig_n = SK.GetEntity({get_entity_ref(EntityRef{uu, 3})});
                    auto en_new_n = SK.GetEntity({get_entity_ref({new_circle_uu, 3})});
                    en_new_n->noEquation = true;
                    auto normal_orig = en_orig_n->NormalGetExprs();
                    auto normal_new = en_new_n->NormalGetExprs();
                    create_eq_n(normal_orig, normal_new, instance);
                }
            }
            else if (it->get_type() == Entity::Type::ARC_3D) {
                auto new_arc_uu = group.get_entity_uuid(uu, instance);
                for (unsigned int pt = 1; pt <= 3; pt++) {
                    auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                    auto en_new_p = get_entity_ref(EntityRef{new_arc_uu, pt});
                    EntityBase *eorig = SK.GetEntity({en_orig_p});
                    EntityBase *enew = SK.GetEntity({en_new_p});
                    ExprVector exorig = eorig->PointGetExprs();
                    ExprVector exnew = enew->PointGetExprs();
                    create_eq3(exorig, exnew, instance);
                }
                {
                    auto en_orig_n = SK.GetEntity({get_entity_ref(EntityRef{uu, 4})});
                    auto en_new_n = SK.GetEntity({get_entity_ref({new_arc_uu, 4})});
                    en_new_n->noEquation = true;
                    auto normal_orig = en_orig_n->NormalGetExprs();
                    auto normal_new = en_new_n->NormalGetExprs();
                    create_eq_n(normal_orig, normal_new, instance);
                }
            }
        }
    }
}

void System::add(const GroupLinearArray &group)
{
    ExprVector direction;
    ExprVector offset;

    auto dx = add_param(group.m_uuid, group.m_dvec.x);
    auto dy = add_param(group.m_uuid, group.m_dvec.y);

    m_param_refs.emplace(dx, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 0});
    m_param_refs.emplace(dy, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 1});

    if (!group.m_active_wrkpl) {
        auto dz = add_param(group.m_uuid, group.m_dvec.z);
        m_param_refs.emplace(dz, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 2});
        direction = ExprVector::From(hParam{dx}, hParam{dy}, hParam{dz});
    }
    else {
        direction = ExprVector::From(Expr::From(hParam{dx}), Expr::From(hParam{dy}), Expr::From(0));
    }

    switch (group.m_offset) {
    case GroupLinearArray::Offset::ZERO:
        offset.x = Expr::From(0);
        offset.y = Expr::From(0);
        offset.z = Expr::From(0);
        break;
    case GroupLinearArray::Offset::ONE:
        offset = direction;
        break;
    case GroupLinearArray::Offset::PARAM: {
        auto ox = add_param(group.m_uuid, group.m_offset_vec.x);
        auto oy = add_param(group.m_uuid, group.m_offset_vec.y);

        m_param_refs.emplace(ox, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 0});
        m_param_refs.emplace(oy, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 1});

        if (!group.m_active_wrkpl) {
            auto oz = add_param(group.m_uuid, group.m_offset_vec.z);
            m_param_refs.emplace(oz, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 2});
            offset = ExprVector::From(hParam{ox}, hParam{oy}, hParam{oz});
        }
        else {
            offset = ExprVector::From(Expr::From(hParam{ox}), Expr::From(hParam{oy}), Expr::From(0));
        }

    } break;
    }

    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;

    auto create_eq2 = [this, &hg, &eqi, &direction, &offset](const ExprVector &exorig, const ExprVector &exnew,
                                                             unsigned int instance) {
        auto direction_scaled = direction.ScaledBy(Expr::From(instance));
        ExprVector shift2 = direction_scaled.Plus(offset);
        AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(shift2.x)), eqi++);
        AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(shift2.y)), eqi++);
    };
    auto create_eq3 = [this, &hg, &eqi, &direction, &offset, &group](const ExprVector &exorig, const ExprVector &exnew,
                                                                     unsigned int instance) {
        auto direction_scaled = direction.ScaledBy(Expr::From(instance));
        ExprVector shift2 = direction_scaled.Plus(offset);
        ExprVector shift3 = shift2;
        if (group.m_active_wrkpl) {
            // transform shift
            auto en_normal = SK.GetEntity({get_entity_ref(EntityRef{group.m_active_wrkpl, 2})});
            shift3 = en_normal->NormalGetExprs().Rotate(shift2);
        }
        AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Plus(shift3.x)), eqi++);
        AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Plus(shift3.y)), eqi++);
        AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z->Plus(shift3.z)), eqi++);
    };
    auto create_eq_n = [this, &hg, &eqi](const ExprQuaternion &normal_orig, const ExprQuaternion &normal_new,
                                         unsigned int instance) {
        AddEq(hg, &m_sys->eq, normal_new.vx->Minus(normal_orig.vx), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vy->Minus(normal_orig.vy), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vz->Minus(normal_orig.vz), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.w->Minus(normal_orig.w), eqi++);
    };

    add_replicate(group, create_eq2, create_eq3, create_eq_n, eqi);
}


void System::add(const GroupPolarArray &group)
{
    if (!group.m_active_wrkpl)
        return;
    auto en_wrkpl = hEntity{get_entity_ref(EntityRef{group.m_active_wrkpl, 0})};
    auto wrkpl = SK.GetEntity(en_wrkpl);

    auto center_point = SK.GetEntity(hEntity{get_entity_ref(EntityRef{group.get_center_point_uuid(), 0})});
    auto angle = add_param(group.m_uuid, group.m_delta_angle / 180 * M_PI);

    ExprVector excenter = center_point->PointGetExprsInWorkplane(en_wrkpl);
    m_param_refs.emplace(angle, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 0, 0});


    Expr *offset_angle = Expr::From(0.);

    switch (group.m_offset) {
    case GroupLinearArray::Offset::ZERO:
        break;
    case GroupLinearArray::Offset::ONE:
        offset_angle = Expr::From(hParam{angle});
        break;
    case GroupLinearArray::Offset::PARAM: {
        auto offset_angle_p = add_param(group.m_uuid, group.m_offset_angle / 180 * M_PI);
        m_param_refs.emplace(offset_angle_p, ParamRef{ParamRef::Type::GROUP, group.m_uuid, 1, 0});
        offset_angle = Expr::From(hParam{offset_angle_p});
    } break;
    }

    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;

    auto create_eq2 = [this, &hg, &eqi, &excenter, &offset_angle,
                       &angle](const ExprVector &exorig, const ExprVector &exnew, unsigned int instance) {
        auto exangle = offset_angle->Plus(Expr::From(hParam{angle})->Times(Expr::From(instance)));
        auto exsin = exangle->Sin();
        auto excos = exangle->Cos();
        auto pc = exorig.Minus(excenter);
        auto prx = pc.x->Times(excos)->Minus(pc.y->Times(exsin));
        auto pry = pc.x->Times(exsin)->Plus(pc.y->Times(excos));
        AddEq(hg, &m_sys->eq, exnew.x->Minus(excenter.x->Plus(prx)), eqi++);
        AddEq(hg, &m_sys->eq, exnew.y->Minus(excenter.y->Plus(pry)), eqi++);
    };


    ExprVector wp = wrkpl->WorkplaneGetOffsetExprs();
    ExprVector wu = wrkpl->Normal()->NormalExprsU();
    ExprVector wv = wrkpl->Normal()->NormalExprsV();
    ExprVector wn = wrkpl->Normal()->NormalExprsN();

    auto create_eq3 = [this, &hg, &eqi, &wp, &wu, &wv, &wn, &excenter, &offset_angle,
                       &angle](const ExprVector &exorig, const ExprVector &exnew, unsigned int instance) {
        auto ev = exorig.Minus(wp);
        auto u = ev.Dot(wu);
        auto v = ev.Dot(wv);
        auto n = ev.Dot(wn);

        auto exangle = offset_angle->Plus(Expr::From(hParam{angle})->Times(Expr::From(instance)));
        auto exsin = exangle->Sin();
        auto excos = exangle->Cos();
        auto pc = ExprVector::From(u, v, Expr::From(0.0)).Minus(excenter);
        auto prx = pc.x->Times(excos)->Minus(pc.y->Times(exsin))->Plus(excenter.x);
        auto pry = pc.x->Times(exsin)->Plus(pc.y->Times(excos))->Plus(excenter.y);
        auto pnew = wp.Plus(wu.ScaledBy(prx)).Plus(wv.ScaledBy(pry)).Plus(wn.ScaledBy(n));
        AddEq(hg, &m_sys->eq, exnew.x->Minus(pnew.x), eqi++);
        AddEq(hg, &m_sys->eq, exnew.y->Minus(pnew.y), eqi++);
        AddEq(hg, &m_sys->eq, exnew.z->Minus(pnew.z), eqi++);
    };

    auto create_eq_n = [this, &hg, &eqi, &offset_angle, &angle, &wn](const ExprQuaternion &normal_orig,
                                                                     const ExprQuaternion &normal_new,
                                                                     unsigned int instance) {
        auto exangle = offset_angle->Plus(Expr::From(hParam{angle})->Times(Expr::From(instance)));
        auto rq = quat_from_axis_angle(wn, exangle);
        auto rot = rq.Times(normal_orig);
        AddEq(hg, &m_sys->eq, normal_new.vx->Minus(rot.vx), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vy->Minus(rot.vy), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vz->Minus(rot.vz), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.w->Minus(rot.w), eqi++);
    };

    add_replicate(group, create_eq2, create_eq3, create_eq_n, eqi);
}

void System::add(const GroupMirrorHV &group)
{
    if (!group.m_active_wrkpl)
        return;
    auto en_wrkpl = hEntity{get_entity_ref(EntityRef{group.m_active_wrkpl, 0})};
    auto wrkpl = SK.GetEntity(en_wrkpl);

    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;

    CreateEq create_eq2;
    if (group.get_type() == Group::Type::MIRROR_HORIZONTAL)
        create_eq2 = [this, &hg, &eqi](const ExprVector &exorig, const ExprVector &exnew, unsigned int instance) {
            AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x), eqi++);
            AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y->Times(Expr::From(instance == 0 ? -1 : 1))), eqi++);
        };
    else
        create_eq2 = [this, &hg, &eqi](const ExprVector &exorig, const ExprVector &exnew, unsigned int instance) {
            AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x->Times(Expr::From(instance == 0 ? -1 : 1))), eqi++);
            AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y), eqi++);
        };

    ExprVector wp = wrkpl->WorkplaneGetOffsetExprs();
    ExprVector wu = wrkpl->Normal()->NormalExprsU();
    ExprVector wv = wrkpl->Normal()->NormalExprsV();

    CreateEq create_eq3;
    if (group.get_type() == Group::Type::MIRROR_HORIZONTAL)
        create_eq3 = [this, &wp, &wv, &hg, &eqi](const ExprVector &exorig, const ExprVector &exnew,
                                                 unsigned int instance) {
            ExprVector pnew;
            if (instance == 0) {
                auto ev = exorig.Minus(wp);
                auto d = ev.Dot(wv);
                pnew = exorig.Minus(wv.ScaledBy(d->Times(Expr::From(2))));
            }
            else {
                pnew = exorig;
            }

            AddEq(hg, &m_sys->eq, exnew.x->Minus(pnew.x), eqi++);
            AddEq(hg, &m_sys->eq, exnew.y->Minus(pnew.y), eqi++);
            AddEq(hg, &m_sys->eq, exnew.z->Minus(pnew.z), eqi++);
        };
    else
        create_eq3 = [this, &wp, &wu, &hg, &eqi](const ExprVector &exorig, const ExprVector &exnew,
                                                 unsigned int instance) {
            ExprVector pnew;
            if (instance == 0) {
                auto ev = exorig.Minus(wp);
                auto d = ev.Dot(wu);
                pnew = exorig.Minus(wu.ScaledBy(d->Times(Expr::From(2))));
            }
            else {
                pnew = exorig;
            }

            AddEq(hg, &m_sys->eq, exnew.x->Minus(pnew.x), eqi++);
            AddEq(hg, &m_sys->eq, exnew.y->Minus(pnew.y), eqi++);
            AddEq(hg, &m_sys->eq, exnew.z->Minus(pnew.z), eqi++);
        };


    CreateEqN create_eq_n = [this, &hg, &eqi](const ExprQuaternion &normal_orig, const ExprQuaternion &normal_new,
                                              unsigned int instance) {
        AddEq(hg, &m_sys->eq, normal_new.vx->Minus(Expr::From(normal_new.vx->Eval())), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vy->Minus(Expr::From(normal_new.vy->Eval())), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.vz->Minus(Expr::From(normal_new.vz->Eval())), eqi++);
        AddEq(hg, &m_sys->eq, normal_new.w->Minus(Expr::From(normal_new.w->Eval())), eqi++);
    };


    add_replicate(group, create_eq2, create_eq3, create_eq_n, eqi);
}


void System::add(const GroupClone &group)
{
    if (!group.m_active_wrkpl)
        return;

    auto hg = hGroup{(uint32_t)group.get_index() + 1};
    unsigned int eqi = 0;

    auto source_wrkpl = hEntity{get_entity_ref(EntityRef{group.m_source_wrkpl, 0})};
    auto dest_wrkpl = hEntity{get_entity_ref(EntityRef{group.m_active_wrkpl, 0})};

    for (const auto &[uu, it] : m_doc.m_entities) {
        if (it->m_group != group.m_source_group)
            continue;
        if (it->get_type() == Entity::Type::LINE_2D) {
            const auto &li = dynamic_cast<const EntityLine2D &>(*it);
            if (li.m_wrkpl != group.m_source_wrkpl)
                continue;
            auto new_line_uu = group.get_entity_uuid(uu);


            for (unsigned int pt = 1; pt <= 2; pt++) {
                auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                auto en_new_p = get_entity_ref(EntityRef{new_line_uu, pt});
                EntityBase *eorig = SK.GetEntity({en_orig_p});
                EntityBase *enew = SK.GetEntity({en_new_p});
                ExprVector exorig = eorig->PointGetExprsInWorkplane(source_wrkpl);
                ExprVector exnew = enew->PointGetExprsInWorkplane(dest_wrkpl);
                AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x), eqi++);
                AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y), eqi++);
            }
        }
        else if (it->get_type() == Entity::Type::BEZIER_2D) {
            const auto &bez = dynamic_cast<const EntityBezier2D &>(*it);
            if (bez.m_wrkpl != group.m_source_wrkpl)
                continue;
            auto new_bez_uu = group.get_entity_uuid(uu);

            for (unsigned int pt = 1; pt <= 4; pt++) {
                auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                auto en_new_p = get_entity_ref(EntityRef{new_bez_uu, pt});
                EntityBase *eorig = SK.GetEntity({en_orig_p});
                EntityBase *enew = SK.GetEntity({en_new_p});
                ExprVector exorig = eorig->PointGetExprsInWorkplane(source_wrkpl);
                ExprVector exnew = enew->PointGetExprsInWorkplane(dest_wrkpl);
                AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x), eqi++);
                AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y), eqi++);
            }
        }
        else if (it->get_type() == Entity::Type::ARC_2D) {
            const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
            if (arc.m_wrkpl != group.m_source_wrkpl)
                continue;
            auto new_arc_uu = group.get_entity_uuid(uu);

            for (unsigned int pt = 1; pt <= 3; pt++) {
                auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                auto en_new_p = get_entity_ref(EntityRef{new_arc_uu, pt});
                EntityBase *eorig = SK.GetEntity({en_orig_p});
                EntityBase *enew = SK.GetEntity({en_new_p});
                ExprVector exorig = eorig->PointGetExprsInWorkplane(source_wrkpl);
                ExprVector exnew = enew->PointGetExprsInWorkplane(dest_wrkpl);
                AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x), eqi++);
                AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y), eqi++);
            }
        }
        else if (it->get_type() == Entity::Type::CIRCLE_2D) {
            const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
            if (circle.m_wrkpl != group.m_source_wrkpl)
                continue;
            auto new_circle_uu = group.get_entity_uuid(uu);

            {
                unsigned int pt = 1;
                auto en_orig_p = get_entity_ref(EntityRef{uu, pt});
                auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, pt});
                EntityBase *eorig = SK.GetEntity({en_orig_p});
                EntityBase *enew = SK.GetEntity({en_new_p});
                ExprVector exorig = eorig->PointGetExprsInWorkplane(source_wrkpl);
                ExprVector exnew = enew->PointGetExprsInWorkplane(dest_wrkpl);
                AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x), eqi++);
                AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y), eqi++);
            }

            {
                auto en_orig_p = get_entity_ref(EntityRef{uu, 0});
                auto en_new_p = get_entity_ref(EntityRef{new_circle_uu, 0});
                EntityBase *eorig = SK.GetEntity({en_orig_p});
                EntityBase *enew = SK.GetEntity({en_new_p});
                AddEq(hg, &m_sys->eq, eorig->CircleGetRadiusExpr()->Minus(enew->CircleGetRadiusExpr()), eqi++);
            }
        }
    }
}


void System::update_document()
{
    for (const auto &[idx, param_ref] : m_param_refs) {
        const auto val = SK.GetParam({idx})->val;
        switch (param_ref.type) {
        case ParamRef::Type::ENTITY:
            m_doc.m_entities.at(param_ref.item)->set_param(param_ref.point, param_ref.axis, val);
            break;
        case ParamRef::Type::GROUP:
            if (m_doc.get_group(param_ref.item).get_type() == Group::Type::EXTRUDE) {
                if (param_ref.point == 0)
                    m_doc.get_group<GroupExtrude>(param_ref.item).m_dvec[param_ref.axis] = val;
                else if (param_ref.point == 1)
                    m_doc.get_group<GroupExtrude>(param_ref.item).m_offset_mul = val;
            }
            else if (m_doc.get_group(param_ref.item).get_type() == Group::Type::LINEAR_ARRAY) {
                if (param_ref.point == 0)
                    m_doc.get_group<GroupLinearArray>(param_ref.item).m_dvec[param_ref.axis] = val;
                else if (param_ref.point == 1)
                    m_doc.get_group<GroupLinearArray>(param_ref.item).m_offset_vec[param_ref.axis] = val;
            }
            else if (m_doc.get_group(param_ref.item).get_type() == Group::Type::POLAR_ARRAY) {
                auto a = val / M_PI * 180.;
                while (a > 360)
                    a -= 360;
                while (a < -360)
                    a += 360;
                if (param_ref.point == 0)
                    m_doc.get_group<GroupPolarArray>(param_ref.item).m_delta_angle = a;
                else if (param_ref.point == 1)
                    m_doc.get_group<GroupPolarArray>(param_ref.item).m_offset_angle = a;
            }
            if (m_doc.get_group(param_ref.item).get_type() == Group::Type::REVOLVE) {
                if (param_ref.point == 0) {
                    auto a = val / M_PI * 180.;
                    while (a > 360)
                        a -= 360;
                    while (a < -360)
                        a += 360;
                    m_doc.get_group<GroupRevolve>(param_ref.item).m_angle = a;
                }
                else if (param_ref.point == 1) {
                    m_doc.get_group<GroupRevolve>(param_ref.item).m_offset_mul = val;
                }
            }
            break;
        }
    }
    for (auto &[idx, uu] : m_constraint_refs) {
        if (auto c = m_doc.get_constraint_ptr<ConstraintSameOrientation>(uu)) {
            const auto val = SK.GetParam(hConstraint{idx}.param(0))->val;
            c->m_val = val;
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintParallel>(uu)) {
            if (!c->m_wrkpl) {
                const auto val = SK.GetParam(hConstraint{idx}.param(0))->val;
                c->m_val = val;
            }
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintPointOnLine>(uu)) {
            const auto val = SK.GetParam(hConstraint{idx}.param(0))->val;
            c->m_val = val;
            c->m_modify_to_satisfy = false;
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintLinesAngle>(uu)) {
            if (c->m_modify_to_satisfy) {
                c->m_angle = SK.constraint.FindById(hConstraint{idx})->valA;
                c->m_modify_to_satisfy = false;
            }
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintPointLineDistance>(uu)) {
            if (c->m_modify_to_satisfy) {
                c->m_distance = SK.constraint.FindById(hConstraint{idx})->valA;
                c->m_modify_to_satisfy = false;
            }
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintPointOnBezier>(uu)) {
            const auto val = SK.GetParam(hConstraint{idx}.param(0))->val;
            c->m_val = val;
        }
        else if (auto c = m_doc.get_constraint_ptr<ConstraintBezierBezierSameCurvature>(uu)) {
            c->m_beta1 = SK.GetParam(hConstraint{idx}.param(0))->val;
            c->m_beta2 = SK.GetParam(hConstraint{idx}.param(0x20000000))->val;
        }
    }
    for (auto &[uu, group] : m_doc.get_groups()) {
        if (auto gp = dynamic_cast<GroupPolarArray *>(group.get()); gp && gp->m_active_wrkpl) {
            auto &en_center = m_doc.get_entity<EntityPoint2D>(gp->get_center_point_uuid());
            gp->m_center = en_center.m_p;
        }
    }
}

void System::add_dragged(const UUID &entity, unsigned int point)
{
    std::set<unsigned int> params;
    const auto entity_type = m_doc.m_entities.at(entity)->get_type();
    switch (entity_type) {
    case Entity::Type::LINE_3D:
    case Entity::Type::LINE_2D:
    case Entity::Type::POINT_2D:
    case Entity::Type::ARC_2D:
    case Entity::Type::ARC_3D:
    case Entity::Type::BEZIER_2D:
    case Entity::Type::BEZIER_3D: {
        for (const auto &[idx, param_ref] : m_param_refs) {
            if (param_ref.type == ParamRef::Type::ENTITY && param_ref.item == entity
                && (param_ref.point == point || point == 0))
                params.insert(idx);
        }
    } break;
    case Entity::Type::CIRCLE_2D: {
        for (const auto &[idx, param_ref] : m_param_refs) {
            if (param_ref.type == ParamRef::Type::ENTITY && param_ref.item == entity && (param_ref.point == point))
                params.insert(idx);
        }
    } break;
    case Entity::Type::CIRCLE_3D: {
        for (const auto &[idx, param_ref] : m_param_refs) {
            if (param_ref.type == ParamRef::Type::ENTITY && param_ref.item == entity && (param_ref.point == 1))
                params.insert(idx);
        }
    } break;
    case Entity::Type::WORKPLANE:
    case Entity::Type::STEP:
    case Entity::Type::CLUSTER:
    case Entity::Type::TEXT:
    case Entity::Type::PICTURE: {
        if (point == 0)
            point = 1;
        for (const auto &[idx, param_ref] : m_param_refs) {
            if (param_ref.type == ParamRef::Type::ENTITY && param_ref.item == entity && (param_ref.point == point))
                params.insert(idx);
        }
    } break;
    default:;
    }


    for (const auto p : params) {
        hParam hp = {p};
        m_sys->dragged.Add(&hp);
    }
}

System::SolveResultWithDof System::solve(std::set<EntityAndPoint> *free_points)
{
    auto &gr = m_doc.get_group(m_solve_group);
    if (gr.get_type() == Group::Type::REFERENCE)
        return {SolveResult::OKAY, 0};

    ::Group g = {};
    g.h.v = gr.get_index() + 1;

    std::cout << "solve group " << gr.m_name << std::endl;
    List<hConstraint> bad = {};
    auto tbegin = clock();
    int dof = -2;
    ::SolveResult how = m_sys->Solve(&g, NULL, &dof, &bad, false, /*andFindFree=*/free_points != nullptr);
    auto tend = clock();
    std::cout << "how " << (int)how << " " << dof << " took " << (double)(tend - tbegin) / CLOCKS_PER_SEC << std::endl
              << std::endl;

    if (free_points) {
        for (const auto &[idx, param_ref] : m_param_refs) {
            if (SK.GetParam({idx})->free) {
                if (param_ref.type == ParamRef::Type::ENTITY) {
                    free_points->emplace(param_ref.item, param_ref.point);
                }
            }
        }
    }

    switch (how) {
    case ::SolveResult::DIDNT_CONVERGE:
        return {SolveResult::DIDNT_CONVERGE, dof};

    case ::SolveResult::REDUNDANT_DIDNT_CONVERGE:
        return {SolveResult::REDUNDANT_DIDNT_CONVERGE, dof};

    case ::SolveResult::OKAY:
        return {SolveResult::OKAY, dof};

    case ::SolveResult::REDUNDANT_OKAY:
        return {SolveResult::REDUNDANT_OKAY, dof};

    case ::SolveResult::TOO_MANY_UNKNOWNS:
        return {SolveResult::TOO_MANY_UNKNOWNS, dof};
    }

    return {SolveResult::OKAY, 0};
}


uint32_t System::add_param(const UUID &group_uu, double value)
{
    auto idx = SK.param.n + 2;

    Param p = {};

    p.h.v = idx;
    p.val = value;
    p.known = group_uu != m_solve_group;
    SK.param.Add(&p);
    if (group_uu == m_solve_group) {
        m_sys->param.Add(&p);
    }
    return idx;
}

uint32_t System::add_param(const UUID &group_uu, const UUID &entity, unsigned int point, unsigned int axis)
{
    const auto p = add_param(group_uu, m_doc.get_entity<Entity>(entity).get_param(point, axis));
    m_param_refs.emplace(p, ParamRef{ParamRef::Type::ENTITY, entity, point, axis});
    return p;
}
unsigned int System::get_entity_ref(const EntityRef &ref)
{
    if (m_entity_refs_r.count(ref))
        return m_entity_refs_r.at(ref);
    unsigned int idx = m_entity_refs.size() + 1;
    m_entity_refs.emplace(idx, ref);
    m_entity_refs_r.emplace(ref, idx);
    return idx;
}


void System::visit(const ConstraintPointsCoincident &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::POINTS_COINCIDENT;
    cb.h.v = c;
    cb.group.v = group;
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_entity1);
    cb.ptB.v = m_entity_refs_r.at(constraint.m_entity2);

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintPointOnLine &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PT_ON_LINE;
    cb.h.v = c;
    cb.group.v = group;
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_point);
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_line, 0});


    Param p = {};
    p.h = cb.h.param(0);
    cb.valP = p.h;
    p.val = constraint.m_val;
    SK.param.Add(&p);

    SK.constraint.Add(&cb);
    if (constraint.m_modify_to_satisfy)
        cb.ModifyToSatisfy();
    m_sys->param.Add(SK.GetParam(p.h));
}

static void AddEq(hConstraint h, IdList<Equation, hEquation> *l, Expr *expr, int index)
{
    Equation eq;
    eq.e = expr;
    eq.h = h.equation(index);
    l->Add(&eq);
}

void System::visit(const ConstraintPointOnCircle &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PT_ON_CIRCLE;
    cb.h.v = c;
    cb.group.v = group;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_point);
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_circle, 0});

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintHV &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    if (constraint.get_type() == Constraint::Type::HORIZONTAL)
        cb.type = ConstraintBase::Type::HORIZONTAL;
    else
        cb.type = ConstraintBase::Type::VERTICAL;
    cb.h.v = c;
    cb.group.v = group;
    cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    cb.ptA.v = m_entity_refs_r.at(constraint.m_entity1);
    cb.ptB.v = m_entity_refs_r.at(constraint.m_entity2);

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintSymmetricHV &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    if (constraint.get_type() == Constraint::Type::SYMMETRIC_HORIZONTAL)
        cb.type = ConstraintBase::Type::SYMMETRIC_HORIZ;
    else
        cb.type = ConstraintBase::Type::SYMMETRIC_VERT;
    cb.h.v = c;
    cb.group.v = group;
    cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    cb.ptA.v = m_entity_refs_r.at(constraint.m_entity1);
    cb.ptB.v = m_entity_refs_r.at(constraint.m_entity2);

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintSymmetricLine &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::SYMMETRIC_LINE;
    cb.h.v = c;
    cb.group.v = group;
    cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    cb.ptA.v = m_entity_refs_r.at(constraint.m_entity1);
    cb.ptB.v = m_entity_refs_r.at(constraint.m_entity2);
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_line, 0});

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintMidpoint &constraint)
{
    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::AT_MIDPOINT;
    cb.h.v = c;
    cb.group.v = group;
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_point);
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_line, 0});

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintPointDistance &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PT_PT_DISTANCE;
    cb.h.v = c;
    cb.group.v = group;
    cb.valA = constraint.m_distance;
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_entity1);
    cb.ptB.v = m_entity_refs_r.at(constraint.m_entity2);

    SK.constraint.Add(&cb);
}


void System::visit(const ConstraintPointDistanceHV &constraint)
{
    if (constraint.m_measurement)
        return;
    const auto c = n_constraint++;

    auto en1 = SK.GetEntity({get_entity_ref(constraint.m_entity1)});
    auto en2 = SK.GetEntity({get_entity_ref(constraint.m_entity2)});
    auto wrkpl = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});

    Expr *u1, *v1, *v2, *u2;
    en1->PointGetExprsInWorkplane({wrkpl}, &u1, &v1);
    en2->PointGetExprsInWorkplane({wrkpl}, &u2, &v2);

    if (constraint.get_type() == Constraint::Type::POINT_DISTANCE_HORIZONTAL) {
        AddEq(hConstraint{c}, &m_sys->eq, u2->Minus(u1)->Minus(Expr::From(constraint.m_distance)), 0);
    }
    else {
        AddEq(hConstraint{c}, &m_sys->eq, v2->Minus(v1)->Minus(Expr::From(constraint.m_distance)), 0);
    }
}

void System::visit(const ConstraintPointDistanceAligned &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto c = n_constraint++;

    auto en1 = SK.GetEntity({get_entity_ref(constraint.m_entity1)});
    auto en2 = SK.GetEntity({get_entity_ref(constraint.m_entity2)});

    ExprVector v1;
    ExprVector v2;
    ExprVector align_vec;

    if (constraint.m_wrkpl) {
        auto en_align = SK.GetEntity({get_entity_ref({constraint.m_align_entity, 0})});
        auto wrkpl = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
        v1 = en1->PointGetExprsInWorkplane({wrkpl});
        v2 = en2->PointGetExprsInWorkplane({wrkpl});
        align_vec = en_align->VectorGetExprsInWorkplane({wrkpl}).WithMagnitude(Expr::From(1));
    }
    else {
        v1 = en1->PointGetExprs();
        v2 = en2->PointGetExprs();

        if (m_doc.get_entity(constraint.m_align_entity).of_type(Entity::Type::WORKPLANE)) {
            auto en_align = SK.GetEntity({get_entity_ref({constraint.m_align_entity, 2})}); // normal
            align_vec = en_align->NormalExprsN().ScaledBy(Expr::From(-1));
        }
        else {
            auto en_align = SK.GetEntity({get_entity_ref({constraint.m_align_entity, 0})}); // line
            align_vec = en_align->VectorGetExprs().WithMagnitude(Expr::From(1));
        }
    }
    AddEq(hConstraint{c}, &m_sys->eq, align_vec.Dot(v1.Minus(v2))->Minus(Expr::From(constraint.m_distance)), 0);
}

void System::visit(const ConstraintPointLineDistance &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto group = get_group_index(constraint);
    const auto c = n_constraint++;

    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PT_LINE_DISTANCE;
    cb.h.v = c;
    cb.group.v = group;
    cb.valA = constraint.m_distance;
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.ptA.v = m_entity_refs_r.at(constraint.m_point);
    cb.entityA.v = m_entity_refs_r.at({constraint.m_line, 0});

    if (constraint.m_modify_to_satisfy)
        cb.ModifyToSatisfy();

    SK.constraint.Add(&cb);
}


System::EntityRef System::get_entity_ref_for_parallel(const UUID &uu) const
{
    auto &en = *m_doc.m_entities.at(uu);
    unsigned int idx = 0;
    switch (en.get_type()) {
    case Entity::Type::WORKPLANE:
    case Entity::Type::STEP:
        idx = 2;
        break;

    case Entity::Type::CIRCLE_3D:
        idx = 3;
        break;

    case Entity::Type::ARC_3D:
        idx = 4;
        break;

    default:
        idx = 0;
    }
    return EntityRef{uu, idx};
}


void System::visit(const ConstraintSameOrientation &constraint)
{
    const auto group = get_group_index(constraint);


    const auto c = n_constraint++;
    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::SAME_ORIENTATION;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(get_entity_ref_for_parallel(constraint.m_entity1));
    cb.entityB.v = m_entity_refs_r.at(get_entity_ref_for_parallel(constraint.m_entity2));
    {
        Param p = {};
        p.h = cb.h.param(0);
        cb.valP = p.h;
        p.val = constraint.m_val;
        SK.param.Add(&p);
        m_sys->param.Add(&p);
    }

    SK.constraint.Add(&cb);
}
void System::visit(const ConstraintParallel &constraint)
{
    const auto group = get_group_index(constraint);


    const auto c = n_constraint++;
    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PARALLEL;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(get_entity_ref_for_parallel(constraint.m_entity1));
    cb.entityB.v = m_entity_refs_r.at(get_entity_ref_for_parallel(constraint.m_entity2));
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    if (!constraint.m_wrkpl) {
        Param p = {};
        p.h = cb.h.param(0);
        cb.valP = p.h;
        p.val = constraint.m_val;
        SK.param.Add(&p);
        m_sys->param.Add(&p);
    }

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintEqualLength &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::EQUAL_LENGTH_LINES;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_entity1, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_entity2, 0});
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;


    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintLengthRatio &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.h.v = c;
    cb.group.v = group;
    const auto ratio = std::clamp(constraint.m_ratio, ConstraintLengthRatio::s_min_ratio,
                                  ConstraintLengthRatio::s_max_ratio);
    cb.valA = ratio;

    const auto ref1 = EntityRef{constraint.m_entity1, 0};
    const auto ref2 = EntityRef{constraint.m_entity2, 0};

    const auto &entity1 = m_doc.get_entity(constraint.m_entity1);
    const auto &entity2 = m_doc.get_entity(constraint.m_entity2);

    const bool arc1 = entity1.of_type(Entity::Type::ARC_2D, Entity::Type::ARC_3D);
    const bool arc2 = entity2.of_type(Entity::Type::ARC_2D, Entity::Type::ARC_3D);

    if (!arc1 && !arc2) {
        cb.type = ConstraintBase::Type::LENGTH_RATIO;
        cb.entityA.v = m_entity_refs_r.at(ref1);
        cb.entityB.v = m_entity_refs_r.at(ref2);
    }
    else if (arc1 && !arc2) {
        cb.type = ConstraintBase::Type::ARC_LINE_LEN_RATIO;
        cb.entityA.v = m_entity_refs_r.at(ref2); // line first
        cb.entityB.v = m_entity_refs_r.at(ref1); // arc second
    }
    else if (!arc1 && arc2) {
        cb.type = ConstraintBase::Type::ARC_LINE_LEN_RATIO;
        cb.entityA.v = m_entity_refs_r.at(ref1); // line first
        cb.entityB.v = m_entity_refs_r.at(ref2); // arc second
        cb.valA = 1.0 / ratio;
        if (!std::isfinite(cb.valA))
            cb.valA = ConstraintLengthRatio::s_max_ratio;
    }
    else {
        cb.type = ConstraintBase::Type::ARC_ARC_LEN_RATIO;
        cb.entityA.v = m_entity_refs_r.at(ref1);
        cb.entityB.v = m_entity_refs_r.at(ref2);
    }

    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintEqualRadius &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::EQUAL_RADIUS;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_entity1, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_entity2, 0});

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintDiameterRadius &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::DIAMETER;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_entity, 0});
    cb.valA = constraint.get_diameter();

    SK.constraint.Add(&cb);
}


void System::visit(const ConstraintWorkplaneNormal &constraint)
{
    auto en_wrkpl_normal = get_entity_ref(EntityRef{constraint.m_wrkpl, 2});
    EntityBase *a = SK.GetEntity({en_wrkpl_normal});
    auto xa = a->NormalGetExprs();
    a->noEquation = true; // we set everything
    auto na = a->NormalGetNum();

    const auto c = n_constraint++;

    AddEq(hConstraint{c}, &m_sys->eq, xa.vx->Minus(Expr::From(na.vx)), 0);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vy->Minus(Expr::From(na.vy)), 1);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vz->Minus(Expr::From(na.vz)), 2);
    AddEq(hConstraint{c}, &m_sys->eq, xa.w->Minus(Expr::From(na.w)), 3);
}

void System::visit(const ConstraintLockRotation &constraint)
{
    auto en_wrkpl_normal = get_entity_ref(EntityRef{constraint.m_entity, 2});
    EntityBase *a = SK.GetEntity({en_wrkpl_normal});
    auto xa = a->NormalGetExprs();
    a->noEquation = true; // we set everything
    auto na = a->NormalGetNum();

    const auto c = n_constraint++;

    AddEq(hConstraint{c}, &m_sys->eq, xa.vx->Minus(Expr::From(na.vx)), 0);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vy->Minus(Expr::From(na.vy)), 1);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vz->Minus(Expr::From(na.vz)), 2);
    AddEq(hConstraint{c}, &m_sys->eq, xa.w->Minus(Expr::From(na.w)), 3);
}

void System::visit(const ConstraintArcArcTangent &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    auto &arc1 = m_doc.get_entity<IEntityInWorkplane>(constraint.m_arc1.entity);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::CURVE_CURVE_TANGENT;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_arc1.entity, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_arc2.entity, 0});
    cb.workplane.v = m_entity_refs_r.at(EntityRef{arc1.get_workplane(), 0});

    cb.other = constraint.m_arc1.point == 2;
    cb.other2 = constraint.m_arc2.point == 2;

    SK.constraint.Add(&cb);
}
void System::visit(const ConstraintArcLineTangent &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::ARC_LINE_TANGENT;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_arc.entity, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_line, 0});
    cb.other = constraint.m_arc.point == 2;

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintLinePointsPerpendicular &constraint)
{
    const auto c = n_constraint++;

    auto en_line = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line, 0})});
    auto en_p1 = SK.GetEntity({get_entity_ref(constraint.m_point_line)});
    auto en_p2 = SK.GetEntity({get_entity_ref(constraint.m_point)});
    auto xl = en_line->VectorGetExprs();
    auto xp1 = en_p1->PointGetExprs();
    auto xp2 = en_p2->PointGetExprs();

    auto xpd = xp1.Minus(xp2);

    AddEq(hConstraint{c}, &m_sys->eq, xpd.Dot(xl), 0);
}

void System::visit(const ConstraintLinesPerpendicular &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::PERPENDICULAR;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_entity1, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_entity2, 0});
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintLinesAngle &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    m_constraint_refs.emplace(c, constraint.m_uuid);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::ANGLE;
    cb.h.v = c;
    cb.group.v = group;
    cb.valA = constraint.m_angle;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_entity1, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_entity2, 0});
    if (constraint.m_wrkpl)
        cb.workplane.v = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    else
        cb.workplane.v = 0;
    cb.other = constraint.m_negative;

    if (constraint.m_modify_to_satisfy)
        cb.ModifyToSatisfy();
    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintPointInPlane &constraint)
{
    const auto c = n_constraint++;

    auto en_p_ref = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line1, 1})});
    auto en_line1 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line1, 0})});
    auto en_line2 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line2, 0})});
    auto en_p = SK.GetEntity({get_entity_ref(constraint.m_point)});
    auto norm = en_line1->VectorGetExprs().Cross(en_line2->VectorGetExprs());
    auto v = en_p->PointGetExprs().Minus(en_p_ref->PointGetExprs());

    AddEq(hConstraint{c}, &m_sys->eq, norm.Dot(v), 0);
}

void System::visit(const ConstraintPointInWorkplane &constraint)
{
    const auto c = n_constraint++;

    auto en_wrkpl_p = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_wrkpl, 1})});
    auto en_wrkpl_n = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_wrkpl, 2})});
    auto en_p = SK.GetEntity({get_entity_ref(constraint.m_point)});
    auto norm = en_wrkpl_n->NormalExprsN();
    auto v = en_p->PointGetExprs().Minus(en_wrkpl_p->PointGetExprs());

    AddEq(hConstraint{c}, &m_sys->eq, norm.Dot(v), 0);
}

void System::visit(const ConstraintPointPlaneDistance &constraint)
{
    if (constraint.m_measurement)
        return;

    const auto c = n_constraint++;

    auto en_p_ref = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line1, 1})});
    auto en_line1 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line1, 0})});
    auto en_line2 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line2, 0})});
    auto en_p = SK.GetEntity({get_entity_ref(constraint.m_point)});
    auto norm = en_line1->VectorGetExprs().Cross(en_line2->VectorGetExprs()).WithMagnitude(Expr::From(1.0));
    auto v = en_p->PointGetExprs().Minus(en_p_ref->PointGetExprs());

    AddEq(hConstraint{c}, &m_sys->eq, norm.Dot(v)->Minus(Expr::From(constraint.m_distance)), 0);
}

void System::visit(const ConstraintBezierLineTangent &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    auto &bez = m_doc.get_entity<EntityBezier2D>(constraint.m_bezier.entity);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::CUBIC_LINE_TANGENT;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_bezier.entity, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_line, 0});
    cb.workplane.v = get_entity_ref(EntityRef{bez.m_wrkpl, 0});
    cb.other = constraint.m_bezier.point == 2;

    SK.constraint.Add(&cb);
}

void System::visit(const ConstraintBezierBezierTangentSymmetric &constraint)
{
    const auto c = n_constraint++;

    // points are p0,p1,p2,p3 p3,p4,p5,p6
    // indices    1 ,3, 4, 2  1 ,3, 4, 2
    // coincidence is at p3

    auto en_wrkpl = get_entity_ref(EntityRef{m_doc.get_entity<EntityBezier2D>(constraint.m_arc1.entity).m_wrkpl, 0});

    auto en_p2 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc1.entity, constraint.m_arc1.point == 2 ? 4u : 3u})});
    auto en_p3 = SK.GetEntity({get_entity_ref(constraint.m_arc1)});
    auto en_p4 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc2.entity, constraint.m_arc2.point == 1 ? 3u : 4u})});

    auto p2 = en_p2->PointGetExprsInWorkplane({en_wrkpl});
    auto p3 = en_p3->PointGetExprsInWorkplane({en_wrkpl});
    auto p4 = en_p4->PointGetExprsInWorkplane({en_wrkpl});

    auto v1 = p2.Minus(p3);
    auto v2 = p3.Minus(p4);

    auto e = v1.Minus(v2);

    AddEq(hConstraint{c}, &m_sys->eq, e.x, 2);
    AddEq(hConstraint{c}, &m_sys->eq, e.y, 3);
}

void System::visit(const ConstraintPointOnBezier &constraint)
{
    const auto c = n_constraint++;

    m_constraint_refs.emplace(c, constraint.m_uuid);

    auto en_wrkpl = get_entity_ref(EntityRef{constraint.m_wrkpl, 0});
    auto en_bez_p1 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line, 1})});
    auto en_bez_p2 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line, 2})});
    auto en_bez_c1 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line, 3})});
    auto en_bez_c2 = SK.GetEntity({get_entity_ref(EntityRef{constraint.m_line, 4})});
    auto en_point = SK.GetEntity({get_entity_ref(constraint.m_point)});

    auto point = en_point->PointGetExprsInWorkplane({en_wrkpl});

    Param p = {};
    p.h = hConstraint{c}.param(0);
    p.val = constraint.m_val;
    SK.param.Add(&p);

    m_sys->param.Add(SK.GetParam(p.h));

    auto t = Expr::From(p.h);
    auto p1 = en_bez_p1->PointGetExprsInWorkplane({en_wrkpl});
    auto p2 = en_bez_p2->PointGetExprsInWorkplane({en_wrkpl});
    auto c1 = en_bez_c1->PointGetExprsInWorkplane({en_wrkpl});
    auto c2 = en_bez_c2->PointGetExprsInWorkplane({en_wrkpl});

    auto one_minus_t = Expr::From(1)->Minus(t);
    auto one_minus_t_square = one_minus_t->Square();
    auto one_minus_t_cube = one_minus_t_square->Times(one_minus_t);
    auto t_square = t->Square();
    auto t_cube = t_square->Times(t);

    auto p_t = p1.ScaledBy(one_minus_t_cube)
                       .Plus(c1.ScaledBy(Expr::From(3)).ScaledBy(one_minus_t_square).ScaledBy(t))
                       .Plus(c2.ScaledBy(Expr::From(3)).ScaledBy(one_minus_t).ScaledBy(t_square))
                       .Plus(p2.ScaledBy(t_cube));

    AddEq(hConstraint{c}, &m_sys->eq, point.x->Minus(p_t.x), 0);
    AddEq(hConstraint{c}, &m_sys->eq, point.y->Minus(p_t.y), 1);
}

void System::visit(const ConstraintBezierBezierSameCurvature &constraint)
{
    const auto c = n_constraint++;
    m_constraint_refs.emplace(c, constraint.m_uuid);

    // points are p0,p1,p2,p3 p3,p4,p5,p6
    // indices    1 ,3, 4, 2  1 ,3, 4, 2
    // coincidence is at p3

    auto en_p1 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc1.entity, constraint.m_arc1.point == 2 ? 3u : 4u})});
    auto en_p2 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc1.entity, constraint.m_arc1.point == 2 ? 4u : 3u})});
    auto en_p3 = SK.GetEntity({get_entity_ref(constraint.m_arc1)});
    auto en_p4 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc2.entity, constraint.m_arc2.point == 1 ? 3u : 4u})});
    auto en_p5 =
            SK.GetEntity({get_entity_ref(EntityRef{constraint.m_arc2.entity, constraint.m_arc2.point == 1 ? 4u : 3u})});

    auto en_wrkpl = get_entity_ref(EntityRef{m_doc.get_entity<EntityBezier2D>(constraint.m_arc1.entity).m_wrkpl, 0});

    Param pb1 = {};
    pb1.h = hConstraint{c}.param(0);
    pb1.val = constraint.m_beta1;
    SK.param.Add(&pb1);

    m_sys->param.Add(SK.GetParam(pb1.h));

    Param pb2 = {};
    pb2.h = hConstraint{c}.param(0x20000000);
    pb2.val = constraint.m_beta2;
    SK.param.Add(&pb2);

    m_sys->param.Add(SK.GetParam(pb2.h));

    auto b1 = Expr::From(pb1.h);
    auto b2 = Expr::From(pb2.h);

    auto p1 = en_p1->PointGetExprsInWorkplane({en_wrkpl});
    auto p2 = en_p2->PointGetExprsInWorkplane({en_wrkpl});
    auto p3 = en_p3->PointGetExprsInWorkplane({en_wrkpl});
    auto p4 = en_p4->PointGetExprsInWorkplane({en_wrkpl});
    auto p5 = en_p5->PointGetExprsInWorkplane({en_wrkpl});

    // see https://en.wikipedia.org/wiki/Composite_B%C3%A9zier_curve#Smoothly_joining_cubic_B%C3%A9ziers
    {
        auto rhs = p3.Plus(p3.Minus(p2).ScaledBy(b1));
        auto e = rhs.Minus(p4);
        AddEq(hConstraint{c}, &m_sys->eq, e.x, 0);
        AddEq(hConstraint{c}, &m_sys->eq, e.y, 1);
    }
    {
        auto rhs = p3.Plus((p3.Minus(p2).ScaledBy(
                                   b1->Times(Expr::From(2))->Plus(b1->Square())->Plus(b2->Times(Expr::From(.5))))))
                           .Plus(p1.Minus(p2).ScaledBy(b1->Square()));

        auto e = rhs.Minus(p5);

        AddEq(hConstraint{c}, &m_sys->eq, e.x, 2);
        AddEq(hConstraint{c}, &m_sys->eq, e.y, 3);
    }
}


void System::visit(const ConstraintBezierArcSameCurvature &constraint)
{
    const auto c = n_constraint++;

    const auto enp_arc = constraint.get_arc(m_doc);
    const auto enp_bezier = constraint.get_bezier(m_doc);
    auto en_arc = SK.GetEntity({get_entity_ref(EntityRef{enp_arc.entity, 0})});
    auto en_wrkpl = get_entity_ref(EntityRef{m_doc.get_entity<EntityArc2D>(enp_arc.entity).m_wrkpl, 0});

    auto en_p1 = SK.GetEntity({get_entity_ref(EntityRef{enp_bezier.entity, 1})});
    auto en_p2 = SK.GetEntity({get_entity_ref(EntityRef{enp_bezier.entity, 2})});
    auto en_c1 = SK.GetEntity({get_entity_ref(EntityRef{enp_bezier.entity, 3})});
    auto en_c2 = SK.GetEntity({get_entity_ref(EntityRef{enp_bezier.entity, 4})});

    auto p1 = en_p1->PointGetExprsInWorkplane({en_wrkpl});
    auto p2 = en_p2->PointGetExprsInWorkplane({en_wrkpl});
    auto c1 = en_c1->PointGetExprsInWorkplane({en_wrkpl});
    auto c2 = en_c2->PointGetExprsInWorkplane({en_wrkpl});

    ExprVector d;
    ExprVector dd;
    Expr *mul = Expr::From(((enp_bezier.point == 1) == (enp_arc.point == 2)) ? 1 : -1);
    if (enp_bezier.point == 1) { // t=0
        // m_p1 * -3 + m_c1 * 3
        d = p1.ScaledBy(Expr::From(-3)).Plus(c1.ScaledBy(Expr::From(3)));

        // 6 * (m_c2 - 2. * m_c1 + m_p1)
        dd = (c2.Minus(c1.ScaledBy(Expr::From(2))).Plus(p1)).ScaledBy(Expr::From(6));
    }
    else { // t=1
        // m_p2 * 3. + m_c2 *-3
        d = p2.ScaledBy(Expr::From(3)).Plus(c2.ScaledBy(Expr::From(-3)));

        // 6 * (m_p2 - 2. * m_c2 + m_c1)
        dd = (p2.Minus(c2.ScaledBy(Expr::From(2))).Plus(c1)).ScaledBy(Expr::From(6));
    }

    // const auto numerator = d.x * dd.y - dd.x * d.y;
    auto numerator = d.x->Times(dd.y)->Minus(dd.x->Times(d.y));
    //  const auto denominator = pow(d.x * d.x + d.y * d.y, 3. / 2);
    auto base = d.x->Times(d.x)->Plus(d.y->Times(d.y));
    auto denominator = (base->Times(base)->Times(base))->Sqrt();

    auto bezeir_radius = denominator->Div(numerator)->Times(mul); // inverse of curvature

    auto arc_radius = en_arc->CircleGetRadiusExpr();

    AddEq(hConstraint{c}, &m_sys->eq, bezeir_radius->Minus(arc_radius), 0);

    // for tangency
    visit(static_cast<const ConstraintArcArcTangent &>(constraint));
}

int System::get_group_index(const UUID &uu) const
{
    return m_doc.get_group(uu).get_index() + 1;
}

int System::get_group_index(const Constraint &constraint) const
{
    return get_group_index(constraint.m_group);
}

int System::get_group_index(const Entity &en) const
{
    return get_group_index(en.m_group);
}


System::~System()
{
    SK.param.Clear();
    SK.entity.Clear();
    SK.constraint.Clear();
    m_sys->Clear();
    FreeAllTemporary();
}

} // namespace dune3d
