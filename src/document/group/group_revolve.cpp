#include "group_revolve.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "util/template_util.hpp"
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
GroupRevolve::GroupRevolve(const UUID &uu) : GroupCircularSweep(uu)
{
}


NLOHMANN_JSON_SERIALIZE_ENUM(GroupRevolve::Mode, {
                                                         {GroupRevolve::Mode::SINGLE, "single"},
                                                         {GroupRevolve::Mode::OFFSET, "offset"},
                                                         {GroupRevolve::Mode::OFFSET_SYMMETRIC, "offset_symmetric"},
                                                 })

GroupRevolve::GroupRevolve(const UUID &uu, const json &j)
    : GroupCircularSweep(uu, j), m_angle(j.at("angle").get<double>()), m_mode(j.at("mode").get<Mode>()),
      m_offset_mul(j.value("offset_mul", -1.))
{
}

json GroupRevolve::serialize() const
{
    auto j = GroupCircularSweep::serialize();
    j["angle"] = m_angle;
    j["mode"] = m_mode;
    if (m_mode != Mode::SINGLE)
        j["offset_mul"] = m_offset_mul;
    return j;
}

std::unique_ptr<Group> GroupRevolve::clone() const
{
    return std::make_unique<GroupRevolve>(*this);
}

static UUID get_side_uuid(GroupRevolve::Side side)
{
    switch (side) {
    case GroupRevolve::Side::TOP:
        return "04b22131-5fe1-4480-8782-44587d727194";
    case GroupRevolve::Side::BOTTOM:
        return "dc49944b-3f2e-4e99-b116-82c783b4df68";
    }
    return {};
}

UUID GroupRevolve::get_entity_uuid(Side side, const UUID &uu) const
{
    return hash_uuids("e47e6775-3fe4-473a-84f8-c2c5578a10af", {m_uuid, m_wrkpl, uu, get_side_uuid(side)});
}

void GroupRevolve::generate(Document &doc) const
{
    generate(doc, Side::TOP);
    if (has_side(Side::BOTTOM))
        generate(doc, Side::BOTTOM);
}

double GroupRevolve::get_side_mul(Side side) const
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

bool GroupRevolve::has_side(Side side) const
{
    switch (side) {
    case Side::TOP:
        return true;
    case Side::BOTTOM:
        return (m_mode != Mode::SINGLE);
    }
    return false;
}

void GroupRevolve::generate(Document &doc, Side side) const
{
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
    const auto angle = m_angle * get_side_mul(side);
    const auto quat = glm::angleAxis(glm::radians(angle), get_direction(doc).value());
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
                new_line.m_p1 = transform(doc, li.m_p1, wrkpl, side);
                new_line.m_p2 = transform(doc, li.m_p2, wrkpl, side);
                new_line.m_group = m_uuid;
                new_line.m_name = "copied";
                new_line.m_generated_from = uu;
                new_line.m_kind = ItemKind::GENRERATED;
            }
        }
        else if (it->get_type() == Entity::Type::CIRCLE_2D) {
            const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
            if (circle.m_wrkpl != m_wrkpl)
                continue;
            auto new_circle_uu = get_entity_uuid(side, uu);
            {
                auto &new_circle = doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                new_circle.m_center = transform(doc, circle.m_center, wrkpl, side);
                new_circle.m_group = m_uuid;
                new_circle.m_normal = quat * wrkpl.m_normal;
                new_circle.m_radius = circle.m_radius;
                new_circle.m_kind = ItemKind::GENRERATED;
                new_circle.m_generated_from = uu;
            }
        }
        else if (it->get_type() == Entity::Type::ARC_2D) {
            const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
            if (arc.m_wrkpl != m_wrkpl)
                continue;
            auto new_arc_uu = get_entity_uuid(side, uu);
            {
                auto &new_arc = doc.get_or_add_entity<EntityArc3D>(new_arc_uu);
                new_arc.m_from = transform(doc, arc.m_from, wrkpl, side);
                new_arc.m_to = transform(doc, arc.m_to, wrkpl, side);
                new_arc.m_center = transform(doc, arc.m_center, wrkpl, side);
                new_arc.m_group = m_uuid;
                new_arc.m_normal = quat * wrkpl.m_normal;
                new_arc.m_kind = ItemKind::GENRERATED;
                new_arc.m_generated_from = uu;
            }
        }
    }
}

glm::dvec3 GroupRevolve::transform_normal(const Document &doc, const glm::dvec3 &v, Side side) const
{
    const auto angle = m_angle * get_side_mul(side);
    const auto ax = glm::angleAxis(glm::radians(angle), get_direction(doc).value());
    return (ax * v);
}

glm::dvec3 GroupRevolve::transform(const Document &doc, const glm::dvec3 &v, Side side) const
{
    const auto angle = m_angle * get_side_mul(side);
    const auto origin = doc.get_point(m_origin);
    const auto p = v - origin;
    const auto ax = glm::angleAxis(glm::radians(angle), get_direction(doc).value());
    return (ax * p) + origin;
}

glm::dvec3 GroupRevolve::transform(const Document &doc, const glm::dvec2 &v, const EntityWorkplane &wrkpl,
                                   Side side) const
{
    return transform(doc, wrkpl.transform(v), side);
}

void GroupRevolve::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

} // namespace dune3d
