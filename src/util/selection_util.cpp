#include "selection_util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/group/group.hpp"
#include "document/constraint/constraint.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "document/constraint/constraint_hv.hpp"
#include "document/constraint/iconstraint_datum.hpp"
#include "canvas/selectable_ref.hpp"
#include "core/idocument_info.hpp"
#include "core/idocument_provider.hpp"
#include <format>
#include <algorithm>

namespace dune3d {

std::set<SelectableRef> filter_selection(const std::set<SelectableRef> &sel, SelectableRef::Type type)
{
    std::set<SelectableRef> out;
    for (const auto &sr : sel) {
        if (sr.type == type)
            out.insert(sr);
    }
    return out;
}

std::set<SelectableRef> entities_from_selection(const std::set<SelectableRef> &sel)
{
    return filter_selection(sel, SelectableRef::Type::ENTITY);
}

std::set<EntityAndPoint> TwoPoints::get_enps() const
{
    return {point1, point2};
}

std::tuple<EntityAndPoint, EntityAndPoint> TwoPoints::get_enps_as_tuple() const
{
    return {point1, point2};
}

std::set<EntityAndPoint> LineAndPoint::get_enps() const
{
    return {{line, 0}, point};
}

std::tuple<EntityAndPoint, EntityAndPoint> LineAndPoint::get_enps_as_tuple() const
{
    return {{line, 0}, point};
}

std::set<EntityAndPoint> LinesAndPoint::get_enps() const
{
    return {{std::get<0>(lines), 0}, {std::get<1>(lines), 0}, point};
}

std::tuple<EntityAndPoint, EntityAndPoint, EntityAndPoint> LinesAndPoint::get_enps_as_tuple() const
{
    return {{std::get<0>(lines), 0}, {std::get<1>(lines), 0}, point};
}


std::optional<TwoPoints> two_points_from_selection(const Document &doc, const std::set<SelectableRef> &sel_all)
{
    const auto sel = entities_from_selection(sel_all);
    if (sel.size() == 1) {
        auto &sr = *sel.begin();
        if (sr.type != SelectableRef::Type::ENTITY)
            return {};
        auto &en = doc.get_entity(sr.item);
        if (en.get_type() == Entity::Type::LINE_2D && sr.point == 0) {
            auto &en2 = dynamic_cast<const EntityLine2D &>(en);
            return TwoPoints{{en2.m_uuid, 1}, {en2.m_uuid, 2}};
        }
        else if (en.get_type() == Entity::Type::LINE_3D && sr.point == 0) {
            auto &en3 = dynamic_cast<const EntityLine3D &>(en);
            return TwoPoints{{en3.m_uuid, 1}, {en3.m_uuid, 2}};
        }
        else {
            return {};
        }
    }
    else if (sel.size() == 2) {
        auto it = sel.begin();
        auto &sr1 = *it++;
        auto &sr2 = *it;

        if (sr1.type != SelectableRef::Type::ENTITY)
            return {};
        if (sr2.type != SelectableRef::Type::ENTITY)
            return {};

        auto &en1 = doc.get_entity(sr1.item);
        auto &en2 = doc.get_entity(sr2.item);
        if (!en1.is_valid_point(sr1.point))
            return {};
        if (!en2.is_valid_point(sr2.point))
            return {};

        return TwoPoints{sr1.get_entity_and_point(), sr2.get_entity_and_point()};
    }
    else {
        return {};
    }
}


std::optional<EntityAndPoint> entity_and_point_from_hover_selection(const Document &doc,
                                                                    const std::optional<SelectableRef> &hover_sel)
{
    if (!hover_sel.has_value())
        return {};
    if (hover_sel->type != SelectableRef::Type::ENTITY)
        return {};
    auto &entity = doc.get_entity(hover_sel->item);
    if (entity.is_valid_point(hover_sel->point))
        return {{hover_sel->item, hover_sel->point}};
    return {};
}

std::optional<LineAndPoint> entity_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel_all,
                                                            const std::set<Entity::Type> &types,
                                                            LineAndPoint::AllowSameEntity allow_same_entity)
{
    const auto sel = entities_from_selection(sel_all);

    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto sr1 = *it++;
    auto sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    if ((allow_same_entity == LineAndPoint::AllowSameEntity::NO) && (sr1.item == sr2.item))
        return {};

    // for an sr to be a line, it must be of LINE2D/3D and point==0
    if (doc.is_valid_point(sr2.get_entity_and_point()))
        std::swap(sr1, sr2);

    auto &sr_point = sr1;
    auto &sr_line = sr2;

    if (!doc.is_valid_point(sr_point.get_entity_and_point()))
        return {};

    if (sr_line.point != 0)
        return {};

    auto &en_line = doc.get_entity(sr_line.item);
    if (!types.contains(en_line.get_type()))
        return {};

    return {{sr_line.item, sr_point.get_entity_and_point()}};
}

std::optional<LineAndPoint> line_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                          LineAndPoint::AllowSameEntity allow_same_entity)
{
    return entity_and_point_from_selection(doc, sel, {Entity::Type::LINE_2D, Entity::Type::LINE_3D}, allow_same_entity);
}

std::optional<LineAndPoint> circle_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                            LineAndPoint::AllowSameEntity allow_same_entity)
{
    using ET = Entity::Type;
    return entity_and_point_from_selection(doc, sel, {ET::ARC_2D, ET::CIRCLE_2D, ET::ARC_3D, ET::CIRCLE_3D},
                                           allow_same_entity);
}

std::optional<LineAndPoint> bezier_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                            LineAndPoint::AllowSameEntity allow_same_entity)
{
    return entity_and_point_from_selection(doc, sel, {Entity::Type::BEZIER_2D, Entity::Type::BEZIER_3D},
                                           allow_same_entity);
}

std::optional<EntityAndPoint> point_from_selection(const Document &doc, const std::set<SelectableRef> &sel_all)
{
    const auto sel = entities_from_selection(sel_all);

    if (sel.size() != 1)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};

    if (!doc.m_entities.contains(sr1.item))
        return {};

    return sr1.get_entity_and_point();
}

std::optional<EntityAndPoint> point_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                                   Entity::Type type)
{
    auto enp = point_from_selection(doc, sel);
    if (!enp)
        return {};

    if (!doc.get_entity(enp->entity).of_type(type))
        return {};

    return enp;
}

std::optional<LinesAndPoint> lines_and_point_from_selection(const Document &doc, const std::set<SelectableRef> &sel_all)
{
    const auto sel = entities_from_selection(sel_all);

    if (sel.size() != 3)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it++;
    auto &sr3 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr3.type != SelectableRef::Type::ENTITY)
        return {};

    std::array<const SelectableRef *, 3> srs = {&sr1, &sr2, &sr3};

    for (size_t i = 0; i < 6; i++) {
        std::ranges::next_permutation(srs);

        auto enp_pt = srs.at(0)->get_entity_and_point();

        LinesAndPoint r;
        if (doc.is_valid_point(enp_pt)) {
            r.point = enp_pt;
            if (srs.at(1)->point != 0)
                return {};
            if (srs.at(2)->point != 0)
                return {};
            if (!doc.get_entity(srs.at(1)->item).of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
                return {};
            if (!doc.get_entity(srs.at(2)->item).of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
                return {};
            r.lines.at(0) = srs.at(1)->item;
            r.lines.at(1) = srs.at(2)->item;
            return r;
        }
    }


    return {};
}


std::optional<UUID> document_from_selection(const std::set<SelectableRef> &sel_all)
{
    const auto sel = filter_selection(sel_all, SelectableRef::Type::DOCUMENT);

    if (sel.size() != 1)
        return {};
    auto &it = *sel.begin();
    if (!it.is_document())
        return {};
    return it.item;
}

static std::string get_workplane_description(const Document &doc, const UUID &wrkpl_uu, const UUID &current_group)
{
    std::string label;
    auto &wrkpl = doc.get_entity(wrkpl_uu);
    auto &wrkpl_group = doc.get_group(wrkpl.m_group);
    label += " in workplane";
    if (wrkpl.has_name() && wrkpl.m_name.size())
        label += " " + wrkpl.m_name;
    if (wrkpl_group.m_uuid == current_group)
        label += " from current group";
    else
        label += " from group " + wrkpl_group.m_name;
    return label;
}

std::string get_selectable_ref_description(IDocumentProvider &prv, const UUID &current_doc, const SelectableRef &sr)
{
    auto &doci = prv.get_idocument_info(current_doc);
    const auto &doc = doci.get_document();
    const UUID current_group = doci.get_current_group();
    std::string label;
    switch (sr.type) {
    case SelectableRef::Type::ENTITY: {
        auto entity = doc.get_entity_ptr(sr.item);
        if (!entity)
            return "not found";
        auto &group = doc.get_group(entity->m_group);
        if (entity->m_construction)
            label = "Construction ";
        if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(entity)) {
            label += entity->get_type_name(Entity::TypeNameStyle::WITHOUT_WORKPLANE);
            if (en_wrkpl->get_workplane() != group.m_active_wrkpl) {
                label += get_workplane_description(doc, en_wrkpl->get_workplane(), current_group);
            }
            else {
                label += " in workplane";
            }
        }
        else {
            label += entity->get_type_name();
        }
        if (entity->has_name() && entity->m_name.size())
            label += " " + entity->m_name;
        if (auto point_name = entity->get_point_name(sr.point); point_name.size())
            label += " (" + point_name + ")";
        if (group.m_uuid == current_group)
            label += " in current group";
        else
            label += " in group " + group.m_name;
    } break;

    case SelectableRef::Type::CONSTRAINT: {
        auto constraint = doc.get_constraint_ptr(sr.item);
        if (!constraint)
            return "not found";

        std::string name = "constraint";
        std::string suffix;
        if (auto dat = dynamic_cast<const IConstraintDatum *>(constraint)) {
            const auto datum = dat->format_datum(dat->get_display_datum(doc));
            if (dat->is_measurement())
                name = "measurement";

            std::string unit;
            if (dat->get_datum_unit() == DatumUnit::MM)
                unit = "mm";

            suffix = std::format(" ({}{})", datum, unit);
        }

        label = constraint->get_type_name() + " " + name + suffix;
        if (auto constraint_wrkpl = dynamic_cast<const IConstraintWorkplane *>(constraint)) {
            if (auto wrkpl_uu = constraint_wrkpl->get_workplane(doc)) {
                label += get_workplane_description(doc, wrkpl_uu, current_group);
            }
        }
    } break;

    case SelectableRef::Type::DOCUMENT: {
        label = "Document " + prv.get_idocument_info(sr.item).get_basename();
    } break;

    case SelectableRef::Type::SOLID_MODEL_EDGE: {
        label = "Solid model edge";
    } break;
    }
    return label;
}

static std::set<EntityAndPoint> coincident_enps_from_enp(const Document &doc, const EntityAndPoint &enp,
                                                         const UUID &group)
{
    UUID wrkpl;
    auto &en = doc.get_entity(enp.entity);
    if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&en))
        wrkpl = en_wrkpl->get_workplane();

    std::set<EntityAndPoint> enps;
    enps.insert(enp);
    std::set<const ConstraintPointsCoincident *> constraints;

    bool done = false;
    while (!done) {
        const auto n_enps = enps.size();
        for (const auto &[uu, el] : doc.m_constraints) {
            if (el->m_group != group)
                continue;
            if (auto cc = dynamic_cast<const ConstraintPointsCoincident *>(el.get())) {
                if (cc->m_wrkpl != wrkpl)
                    continue;
                if (enps.contains(cc->m_entity1) || enps.contains(cc->m_entity2)) {
                    auto &en1 = doc.get_entity(cc->m_entity1.entity);
                    auto &en2 = doc.get_entity(cc->m_entity2.entity);
                    if (auto en_wrkpl1 = dynamic_cast<const IEntityInWorkplane *>(&en1)) {
                        if (en_wrkpl1->get_workplane() != wrkpl)
                            continue;
                    }
                    if (auto en_wrkpl2 = dynamic_cast<const IEntityInWorkplane *>(&en2)) {
                        if (en_wrkpl2->get_workplane() != wrkpl)
                            continue;
                    }
                    enps.insert(cc->m_entity1);
                    enps.insert(cc->m_entity2);
                }
            }
        }
        if (n_enps == enps.size())
            done = true;
    }
    return enps;
}


std::optional<TwoPoints> joint_from_selection(const Document &doc, const std::set<SelectableRef> &sel_all,
                                              const std::set<Entity::Type> &types, const UUID &group)
{
    const auto sel = entities_from_selection(sel_all);
    if (sel.size() == 1 && doc.is_valid_point(sel.begin()->get_entity_and_point())) {
        const auto enp = sel.begin()->get_entity_and_point();
        auto other_enps = coincident_enps_from_enp(doc, enp, group);
        if (other_enps.size() != 2)
            return {};
        auto it = other_enps.begin();
        auto enp1 = *it++;
        auto enp2 = *it;
        if (!types.contains(doc.get_entity(enp1.entity).get_type()))
            return {};
        if (!types.contains(doc.get_entity(enp2.entity).get_type()))
            return {};
        return {{enp1, enp2}};
    }
    else if (sel.size() == 3) {
        auto it_sel = sel.begin();
        auto enp1 = (it_sel++)->get_entity_and_point();
        auto enp2 = (it_sel++)->get_entity_and_point();
        auto enp3 = (it_sel++)->get_entity_and_point();

        EntityAndPoint enp_joint;
        UUID entity1;
        UUID entity2;
        if (doc.is_valid_point(enp1) && enp2.point == 0 && enp3.point == 0) {
            enp_joint = enp1;
            entity1 = enp2.entity;
            entity2 = enp3.entity;
        }
        else if (doc.is_valid_point(enp2) && enp1.point == 0 && enp3.point == 0) {
            enp_joint = enp2;
            entity1 = enp1.entity;
            entity2 = enp3.entity;
        }
        else if (doc.is_valid_point(enp3) && enp1.point == 0 && enp2.point == 0) {
            enp_joint = enp3;
            entity1 = enp1.entity;
            entity2 = enp2.entity;
        }
        else {
            return {};
        }

        if (!types.contains(doc.get_entity(entity1).get_type()))
            return {};
        if (!types.contains(doc.get_entity(entity2).get_type()))
            return {};

        const auto other_enps = coincident_enps_from_enp(doc, enp_joint, group);
        const auto it1 = std::ranges::find_if(other_enps, [&entity1](auto &it) { return it.entity == entity1; });
        const auto it2 = std::ranges::find_if(other_enps, [&entity2](auto &it) { return it.entity == entity2; });
        if (it1 == other_enps.end() || it2 == other_enps.end())
            return {};

        return {{*it1, *it2}};
    }

    return {};
}

std::list<UUID> entities_from_selection(const Document &doc, const std::set<SelectableRef> &sel,
                                        const std::set<Entity::Type> &types)
{
    std::list<UUID> r;
    for (const auto &sr : sel) {
        if (sr.type != SelectableRef::Type::ENTITY)
            return {};
        if (sr.point != 0)
            return {};
        auto &en = doc.get_entity(sr.item);
        if (types.contains(en.get_type()))
            r.push_back(sr.item);
        else
            return {};
    }
    return r;
}

bool entity_is_constrained_hv(const Document &doc, const UUID &uu, const UUID &wrkpl)
{
    if (!wrkpl)
        return false;
    auto en = doc.get_entity_ptr<EntityLine2D>(uu);
    if (!en)
        return false;
    if (en->m_wrkpl != wrkpl)
        return false;
    for (auto constraint : doc.find_constraints({{uu, 1}})) {
        const auto hv = dynamic_cast<const ConstraintHV *>(constraint);
        if (!hv)
            continue;
        if (hv->m_wrkpl != wrkpl)
            continue;

        if ((hv->m_entity1 == EntityAndPoint{uu, 1} && hv->m_entity2 == EntityAndPoint{uu, 2})
            || (hv->m_entity1 == EntityAndPoint{uu, 2} && hv->m_entity2 == EntityAndPoint{uu, 1}))
            return true;
    }

    return false;
}


} // namespace dune3d
