#include "group_linear_array.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_circle3d.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/solid_model.hpp"

namespace dune3d {
GroupLinearArray::GroupLinearArray(const UUID &uu) : Group(uu)
{
}

NLOHMANN_JSON_SERIALIZE_ENUM(GroupLinearArray::Offset, {
                                                               {GroupLinearArray::Offset::ONE, "one"},
                                                               {GroupLinearArray::Offset::ZERO, "zero"},
                                                               {GroupLinearArray::Offset::PARAM, "param"},
                                                       })

GroupLinearArray::GroupLinearArray(const UUID &uu, const json &j)
    : Group(uu, j), m_source_group(j.at("source_group").get<UUID>()), m_dvec(j.at("dvec").get<glm::dvec3>()),
      m_count(j.at("count").get<unsigned int>()), m_offset(j.at("offset").get<Offset>()),
      m_offset_vec(j.at("offset_vec").get<glm::dvec3>())
{
}

json GroupLinearArray::serialize() const
{
    auto j = Group::serialize();
    j["source_group"] = m_source_group;
    j["count"] = m_count;
    j["dvec"] = m_dvec;
    j["offset"] = m_offset;
    j["offset_vec"] = m_offset_vec;
    return j;
}

const SolidModel *GroupLinearArray::get_solid_model() const
{
    return m_solid_model.get();
}

void GroupLinearArray::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

UUID GroupLinearArray::get_entity_uuid(const UUID &uu, unsigned int instance) const
{
    return hash_uuids("dee4fd38-6aa6-414f-bd45-524cf97b860b", {m_uuid, uu},
                      {reinterpret_cast<const uint8_t *>(&instance), sizeof(instance)});
}

glm::dvec3 GroupLinearArray::get_shift(unsigned int instance) const
{
    glm::dvec3 offset;
    switch (m_offset) {
    case Offset::ZERO:
        offset = {0, 0, 0};
        break;
    case Offset::ONE:
        offset = m_dvec;
        break;
    case Offset::PARAM:
        offset = m_offset_vec;
        break;
    }
    return offset + m_dvec * (double)instance;
}

glm::dvec3 GroupLinearArray::get_shift3(const Document &doc, unsigned int instance) const
{
    if (m_active_wrkpl) {
        const auto sh = get_shift2(instance);
        auto &wrkpl = doc.get_entity<EntityWorkplane>(m_active_wrkpl);
        return wrkpl.transform_relative(sh);
    }
    else {
        return get_shift(instance);
    }
}


glm::dvec2 GroupLinearArray::get_shift2(unsigned int instance) const
{
    auto sh = get_shift(instance);
    return {sh.x, sh.y};
}


void GroupLinearArray::generate(Document &doc) const
{
    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        for (unsigned int instance = 0; instance < m_count; instance++) {
            const auto shift2 = get_shift2(instance);
            const auto shift = get_shift3(doc, instance);
            if (it->get_type() == Entity::Type::LINE_2D) {
                const auto &li = dynamic_cast<const EntityLine2D &>(*it);
                if (li.m_wrkpl != m_active_wrkpl)
                    continue;
                {
                    auto new_line_uu = get_entity_uuid(uu, instance);
                    auto &new_line = doc.get_or_add_entity<EntityLine2D>(new_line_uu);
                    new_line.m_p1 = li.m_p1 + shift2;
                    new_line.m_p2 = li.m_p2 + shift2;
                    new_line.m_kind = ItemKind::GENRERATED;
                    new_line.m_group = m_uuid;
                    new_line.m_wrkpl = li.m_wrkpl;
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_2D) {
                const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
                if (circle.m_wrkpl != m_active_wrkpl)
                    continue;
                {
                    auto new_circle_uu = get_entity_uuid(uu, instance);
                    auto &new_circle = doc.get_or_add_entity<EntityCircle2D>(new_circle_uu);
                    new_circle.m_center = circle.m_center + shift2;
                    new_circle.m_radius = circle.m_radius;
                    new_circle.m_kind = ItemKind::GENRERATED;
                    new_circle.m_group = m_uuid;
                    new_circle.m_wrkpl = circle.m_wrkpl;
                }
            }
            else if (it->get_type() == Entity::Type::ARC_2D) {
                const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
                if (arc.m_wrkpl != m_active_wrkpl)
                    continue;
                {
                    auto new_arc_uu = get_entity_uuid(uu, instance);
                    auto &new_arc = doc.get_or_add_entity<EntityArc2D>(new_arc_uu);
                    new_arc.m_no_radius_constraint = true;
                    new_arc.m_from = arc.m_from + shift2;
                    new_arc.m_to = arc.m_to + shift2;
                    new_arc.m_center = arc.m_center + shift2;
                    new_arc.m_kind = ItemKind::GENRERATED;
                    new_arc.m_group = m_uuid;
                    new_arc.m_wrkpl = arc.m_wrkpl;
                }
            }
            else if (it->get_type() == Entity::Type::LINE_3D) {
                const auto &li = dynamic_cast<const EntityLine3D &>(*it);
                {
                    auto new_line_uu = get_entity_uuid(uu, instance);
                    auto &new_line = doc.get_or_add_entity<EntityLine3D>(new_line_uu);
                    new_line.m_p1 = li.m_p1 + shift;
                    new_line.m_p2 = li.m_p2 + shift;
                    new_line.m_kind = ItemKind::GENRERATED;
                    new_line.m_group = m_uuid;
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_3D) {
                const auto &circle = dynamic_cast<const EntityCircle3D &>(*it);
                {
                    auto new_circle_uu = get_entity_uuid(uu, instance);
                    auto &new_circle = doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                    new_circle.m_center = circle.m_center + shift;
                    new_circle.m_radius = circle.m_radius;
                    new_circle.m_kind = ItemKind::GENRERATED;
                    new_circle.m_group = m_uuid;
                    new_circle.m_normal = circle.m_normal;
                }
            }
            else if (it->get_type() == Entity::Type::ARC_3D) {
                const auto &arc = dynamic_cast<const EntityArc3D &>(*it);
                {
                    auto new_arc_uu = get_entity_uuid(uu, instance);
                    auto &new_arc = doc.get_or_add_entity<EntityArc3D>(new_arc_uu);
                    new_arc.m_from = arc.m_from + shift;
                    new_arc.m_to = arc.m_to + shift;
                    new_arc.m_center = arc.m_center + shift;
                    new_arc.m_kind = ItemKind::GENRERATED;
                    new_arc.m_group = m_uuid;
                    new_arc.m_normal = arc.m_normal;
                }
            }
        }
    }
}

std::set<UUID> GroupLinearArray::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    return r;
}

std::set<UUID> GroupLinearArray::get_referenced_groups(const Document &doc) const
{
    auto r = Group::get_referenced_groups(doc);
    r.insert(m_source_group);
    return r;
}

std::set<UUID> GroupLinearArray::get_required_entities(const Document &doc) const
{
    return {};
}

std::set<UUID> GroupLinearArray::get_required_groups(const Document &doc) const
{
    return {m_source_group};
}

std::list<GroupStatusMessage> GroupLinearArray::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_array_messages.begin(), m_array_messages.end());
    return msg;
}

std::unique_ptr<Group> GroupLinearArray::clone() const
{
    return std::make_unique<GroupLinearArray>(*this);
}

} // namespace dune3d
