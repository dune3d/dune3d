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
#include "document/constraint/all_constraints.hpp"
#include "document/group/group.hpp"
#include "document/group/group_extrude.hpp"
#include <array>
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

System::System(Document &doc, const UUID &grp)
    : m_sys(std::make_unique<SolveSpace::System>()), m_doc(doc), m_solve_group(grp), m_lock(s_sys_mutex)
{
    for (auto &[uu, constraint] : m_doc.m_constraints) {
        if (constraint->m_group == m_solve_group)
            if (auto ps = dynamic_cast<const IConstraintPreSolve *>(constraint.get()))
                ps->pre_solve(m_doc);
    }

    for (const auto &[uu, entity] : m_doc.m_entities) {
        entity->accept(*this);
    }
    for (const auto &[uu, constraint] : m_doc.m_constraints) {
        if (constraint->m_group != m_solve_group)
            continue;
        constraint->accept(*this);
    }
    for (const auto &[uu, group] : m_doc.get_groups()) {
        if (uu != m_solve_group)
            continue;
        switch (group->get_type()) {
        case Group::Type::EXTRUDE:
            add(dynamic_cast<const GroupExtrude &>(*group));
            break;
        default:;
        }
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

    auto &source_arc = m_doc.get_entity<EntityArc2D>(arc.m_source);

    auto e = get_entity_ref(EntityRef{arc.m_uuid, 0});
    EntityBase eb = {};
    eb.type = EntityBase::Type::ARC_OF_CIRCLE;
    eb.h.v = e;
    eb.group.v = group;
    eb.normal.v = {get_entity_ref(EntityRef{source_arc.m_wrkpl, 2})};
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
            auto dl = add_param(group.m_uuid, glm::dot(group.m_dvec, wrkpl.get_normal()));
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

            AddEq(hg, &m_sys->eq, exp2.x->Minus(exp1.x)->Minus(direction.x), eqi++);
            AddEq(hg, &m_sys->eq, exp2.y->Minus(exp1.y)->Minus(direction.y), eqi++);
            AddEq(hg, &m_sys->eq, exp2.z->Minus(exp1.z)->Minus(direction.z), eqi++);
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
                        AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x)->Minus(direction.x), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y)->Minus(direction.y), eqi++);
                        AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z)->Minus(direction.z), eqi++);
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
                    AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x)->Minus(direction.x), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y)->Minus(direction.y), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z)->Minus(direction.z), eqi++);
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
                    AddEq(hg, &m_sys->eq, exnew.x->Minus(exorig.x)->Minus(direction.x), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.y->Minus(exorig.y)->Minus(direction.y), eqi++);
                    AddEq(hg, &m_sys->eq, exnew.z->Minus(exorig.z)->Minus(direction.z), eqi++);
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
                    auto eqo = eorig->NormalGetExprs();
                    auto eqn = enew->NormalGetExprs();
                    AddEq(hg, &m_sys->eq, eqo.vx->Minus(eqn.vx), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vy->Minus(eqn.vy), eqi++);
                    AddEq(hg, &m_sys->eq, eqo.vz->Minus(eqn.vz), eqi++);
                }
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
            break;
        }
    }
    for (auto &[idx, uu] : m_constraint_refs) {
        const auto val = SK.GetParam(hConstraint{idx}.param(0))->val;
        if (auto c = m_doc.get_constraint_ptr<ConstraintSameOrientation>(uu))
            c->m_val = val;
        else if (auto c = m_doc.get_constraint_ptr<ConstraintParallel>(uu))
            c->m_val = val;
        else if (auto c = m_doc.get_constraint_ptr<ConstraintPointOnLine>(uu)) {
            c->m_val = val;
            c->m_modify_to_satisfy = false;
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
    case Entity::Type::ARC_2D:
    case Entity::Type::ARC_3D: {
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
    case Entity::Type::STEP: {
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

bool System::solve()
{
    auto &gr = m_doc.get_group(m_solve_group);
    if (gr.get_type() == Group::Type::REFERENCE)
        return false;

    ::Group g = {};
    g.h.v = gr.get_index() + 1;

    std::cout << "solve group " << gr.m_name << std::endl;
    List<hConstraint> bad = {};
    auto tbegin = clock();
    int dof = -2;
    SolveResult how = m_sys->Solve(&g, NULL, &dof, &bad, false, /*andFindFree=*/false);
    auto tend = clock();
    std::cout << "how " << (int)how << " " << dof << " took " << (double)(tend - tbegin) / CLOCKS_PER_SEC << std::endl
              << std::endl;

    gr.m_dof = dof;
    gr.m_solve_messages.clear();
    switch (how) {
    case SolveResult::DIDNT_CONVERGE:
        gr.m_solve_messages.emplace_back(GroupStatusMessage::Status::ERR, "Solver did not converge");
        break;
    case SolveResult::REDUNDANT_DIDNT_CONVERGE:
        gr.m_solve_messages.emplace_back(GroupStatusMessage::Status::ERR,
                                         "Solver did not converge, redundant constraints");
        break;
    case SolveResult::OKAY:
        break;
    case SolveResult::REDUNDANT_OKAY:
        gr.m_solve_messages.emplace_back(GroupStatusMessage::Status::WARN, "Redundant constraints");
        break;
    case SolveResult::TOO_MANY_UNKNOWNS:
        gr.m_solve_messages.emplace_back(GroupStatusMessage::Status::ERR, "Too many unknowns");
        break;
    }


    return how == SolveResult::OKAY;
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
    const auto c = n_constraint++;

    EntityBase *circle = SK.GetEntity({m_entity_refs_r.at(EntityRef{constraint.m_circle, 0})});
    EntityBase *point = SK.GetEntity({m_entity_refs_r.at(constraint.m_point)});

    auto center = SK.GetEntity(circle->point[0])->PointGetExprs();
    auto normal = SK.GetEntity(circle->normal)->NormalExprsN();
    auto pointex = point->PointGetExprs();

    if (point->workplane == ::Entity::FREE_IN_3D)
        AddEq(hConstraint{c}, &m_sys->eq, normal.Dot(pointex.Minus(center)), 0);
    AddEq(hConstraint{c}, &m_sys->eq, (pointex.Minus(center)).Magnitude()->Minus(circle->CircleGetRadiusExpr()), 1);
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

System::EntityRef System::get_entity_ref_for_parallel(const UUID &uu) const
{
    auto &en = *m_doc.m_entities.at(uu);
    unsigned int idx = 0;
    switch (en.get_type()) {
    case Entity::Type::WORKPLANE:
        idx = 2;
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
    auto na = a->NormalGetNum();

    const auto c = n_constraint++;

    AddEq(hConstraint{c}, &m_sys->eq, xa.vx->Minus(Expr::From(na.vx)), 0);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vy->Minus(Expr::From(na.vy)), 1);
    AddEq(hConstraint{c}, &m_sys->eq, xa.vz->Minus(Expr::From(na.vz)), 2);
    //    AddEq(hConstraint{c}, &m_sys->eq, xa.vx->Minus(Expr::From(na.w)), 3);
}

void System::visit(const ConstraintArcArcTangent &constraint)
{
    const auto group = get_group_index(constraint);

    const auto c = n_constraint++;

    auto &arc1 = m_doc.get_entity<EntityArc2D>(constraint.m_arc1.entity);

    ConstraintBase cb = {};
    cb.type = ConstraintBase::Type::CURVE_CURVE_TANGENT;
    cb.h.v = c;
    cb.group.v = group;
    cb.entityA.v = m_entity_refs_r.at(EntityRef{constraint.m_arc1.entity, 0});
    cb.entityB.v = m_entity_refs_r.at(EntityRef{constraint.m_arc2.entity, 0});
    cb.workplane.v = m_entity_refs_r.at(EntityRef{arc1.m_wrkpl, 0});

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
    FreeAllTemporary();
}

} // namespace dune3d
