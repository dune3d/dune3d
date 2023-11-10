#include "group_extrude.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/solid_model.hpp"

namespace dune3d {
GroupExtrude::GroupExtrude(const UUID &uu) : GroupSweep(uu)
{
}

NLOHMANN_JSON_SERIALIZE_ENUM(GroupExtrude::Direction, {
                                                              {GroupExtrude::Direction::NORMAL, "normal"},
                                                              {GroupExtrude::Direction::ARBITRARY, "arbitrary"},
                                                      })
NLOHMANN_JSON_SERIALIZE_ENUM(GroupExtrude::Mode, {
                                                         {GroupExtrude::Mode::SINGLE, "single"},
                                                         {GroupExtrude::Mode::OFFSET, "offset"},
                                                         {GroupExtrude::Mode::OFFSET_SYMMETRIC, "offset_symmetric"},
                                                 })

GroupExtrude::GroupExtrude(const UUID &uu, const json &j)
    : GroupSweep(uu, j), m_dvec(j.at("dvec").get<glm::dvec3>()), m_mode(j.value("mode", Mode::SINGLE)),
      m_offset_mul(j.value("offset_mul", -1.))
{
    if (j.contains("direction"))
        j.at("direction").get_to(m_direction);
    // j.at("parallel_constraint_vals").get_to(m_parallel_constraint_vals);
}

json GroupExtrude::serialize() const
{
    auto j = GroupSweep::serialize();
    j["dvec"] = m_dvec;
    j["direction"] = m_direction;
    j["mode"] = m_mode;
    if (m_mode != Mode::SINGLE)
        j["offset_mul"] = m_offset_mul;
    // j["parallel_constraint_vals"] = m_parallel_constraint_vals;
    return j;
}

std::unique_ptr<Group> GroupExtrude::clone() const
{
    return std::make_unique<GroupExtrude>(*this);
}

static UUID get_side_uuid(GroupExtrude::Side side)
{
    switch (side) {
    case GroupExtrude::Side::TOP:
        return "04b22131-5fe1-4480-8782-44587d727194";
    case GroupExtrude::Side::BOTTOM:
        return "dc49944b-3f2e-4e99-b116-82c783b4df68";
    }
    return {};
}

UUID GroupExtrude::get_leader_line_uuid(Side side) const
{
    return hash_uuids("f4a51b25-ddad-402d-ad67-fbaeeac4507e", {m_uuid, m_wrkpl, get_side_uuid(side)});
}

UUID GroupExtrude::get_entity_uuid(Side side, const UUID &uu) const
{
    return hash_uuids("e47e6775-3fe4-473a-84f8-c2c5578a10af", {m_uuid, m_wrkpl, uu, get_side_uuid(side)});
}

UUID GroupExtrude::get_extrusion_line_uuid(Side side, const UUID &uu, unsigned int pt) const
{
    return hash_uuids("e47e6775-3fe4-473a-84f8-c2c5578a10af", {m_uuid, m_wrkpl, uu, get_side_uuid(side)},
                      {reinterpret_cast<const uint8_t *>(&pt), sizeof(pt)});
}

void GroupExtrude::generate(Document &doc) const
{
    generate(doc, Side::TOP);
    if (has_side(Side::BOTTOM))
        generate(doc, Side::BOTTOM);
}

double GroupExtrude::get_side_mul(Side side) const
{
    switch (side) {
    case Side::TOP:
        return 1;
    case Side::BOTTOM:
        if (m_mode == Mode::OFFSET_SYMMETRIC)
            return -1;
        else
            return m_offset_mul;
    }
    return 1;
}

bool GroupExtrude::has_side(Side side) const
{
    switch (side) {
    case Side::TOP:
        return true;
    case Side::BOTTOM:
        return (m_mode != Mode::SINGLE);
    }
    return false;
}

void GroupExtrude::generate(Document &doc, Side side) const
{
    auto leader_line_uu = get_leader_line_uuid(side);
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    const double mul = get_side_mul(side);
    const auto dvec = m_dvec * mul;
    {
        auto &leader = doc.get_or_add_entity<EntityLine3D>(leader_line_uu);
        leader.m_p1 = wrkpl.m_origin;
        leader.m_p2 = wrkpl.m_origin + dvec;
        leader.m_group = m_uuid;
        leader.m_name = "leader";
        leader.m_kind = ItemKind::GENRERATED;
    }
    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        if (it->get_type() == Entity::Type::LINE_2D) {
            const auto &li = dynamic_cast<const EntityLine2D &>(*it);
            if (li.m_wrkpl != m_wrkpl)
                continue;

            {
                auto new_line_uu = get_entity_uuid(side, uu);

                auto &new_line = doc.get_or_add_entity<EntityLine3D>(new_line_uu);
                new_line.m_p1 = wrkpl.transform(li.m_p1) + dvec;
                new_line.m_p2 = wrkpl.transform(li.m_p2) + dvec;
                new_line.m_group = m_uuid;
                new_line.m_name = "copied";
                new_line.m_kind = ItemKind::GENRERATED;
            }
            for (unsigned int pt = 1; pt <= 2; pt++) {
                auto new_line_uu = get_extrusion_line_uuid(side, uu, pt);

                auto &new_line = doc.get_or_add_entity<EntityLine3D>(new_line_uu);
                new_line.m_p1 = wrkpl.transform(li.get_point(pt, doc));
                new_line.m_p2 = wrkpl.transform(li.get_point(pt, doc)) + dvec;
                new_line.m_group = m_uuid;
                new_line.m_name = "Extrusion" + std::to_string(pt);
                new_line.m_move_instead.clear();
                new_line.m_move_instead.emplace(std::piecewise_construct, std::forward_as_tuple(1),
                                                std::forward_as_tuple(li.m_uuid, pt));
                //  new_line.m_wrkpl = new_wrkpl_uu;

                new_line.m_kind = ItemKind::GENRERATED;
            }
        }
        else if (it->get_type() == Entity::Type::ARC_2D) {
            const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
            if (arc.m_wrkpl != m_wrkpl)
                continue;
            auto new_arc_uu = get_entity_uuid(side, uu);
            {
                auto &new_arc = doc.get_or_add_entity<EntityArc3D>(new_arc_uu);
                new_arc.m_from = wrkpl.transform(arc.m_from) + dvec;
                new_arc.m_to = wrkpl.transform(arc.m_to) + dvec;
                new_arc.m_center = wrkpl.transform(arc.m_center) + dvec;
                new_arc.m_group = m_uuid;
                new_arc.m_normal = wrkpl.m_normal;
                new_arc.m_kind = ItemKind::GENRERATED;
            }
        }
        else if (it->get_type() == Entity::Type::CIRCLE_2D) {
            const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
            if (circle.m_wrkpl != m_wrkpl)
                continue;
            auto new_circle_uu = get_entity_uuid(side, uu);
            {
                auto &new_circle = doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                new_circle.m_center = wrkpl.transform(circle.m_center) + dvec;
                new_circle.m_group = m_uuid;
                new_circle.m_normal = wrkpl.m_normal;
                new_circle.m_radius = circle.m_radius;
                new_circle.m_kind = ItemKind::GENRERATED;
            }
            {
                unsigned int pt = 1;
                auto new_line_uu = get_extrusion_line_uuid(side, uu, pt);

                auto &new_line = doc.get_or_add_entity<EntityLine3D>(new_line_uu);
                new_line.m_p1 = wrkpl.transform(circle.get_point(pt, doc));
                new_line.m_p2 = wrkpl.transform(circle.get_point(pt, doc)) + dvec;
                new_line.m_group = m_uuid;
                new_line.m_name = "Extrusion" + std::to_string(pt);
                new_line.m_move_instead.clear();
                new_line.m_move_instead.emplace(std::piecewise_construct, std::forward_as_tuple(1),
                                                std::forward_as_tuple(circle.m_uuid, pt));

                new_line.m_kind = ItemKind::GENRERATED;
            }
        }
    }
}

void GroupExtrude::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

} // namespace dune3d
