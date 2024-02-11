#include "group_array.hpp"
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
GroupArray::GroupArray(const UUID &uu) : Group(uu)
{
}

NLOHMANN_JSON_SERIALIZE_ENUM(GroupArray::Offset, {
                                                         {GroupArray::Offset::ONE, "one"},
                                                         {GroupArray::Offset::ZERO, "zero"},
                                                         {GroupArray::Offset::PARAM, "param"},
                                                 })

GroupArray::GroupArray(const UUID &uu, const json &j)
    : Group(uu, j), m_source_group(j.at("source_group").get<UUID>()), m_count(j.at("count").get<unsigned int>()),
      m_offset(j.at("offset").get<Offset>())
{
}

json GroupArray::serialize() const
{
    auto j = Group::serialize();
    j["source_group"] = m_source_group;
    j["count"] = m_count;
    j["offset"] = m_offset;
    return j;
}

const SolidModel *GroupArray::get_solid_model() const
{
    return m_solid_model.get();
}

UUID GroupArray::get_entity_uuid(const UUID &uu, unsigned int instance) const
{
    return hash_uuids("dee4fd38-6aa6-414f-bd45-524cf97b860b", {m_uuid, uu},
                      {reinterpret_cast<const uint8_t *>(&instance), sizeof(instance)});
}

void GroupArray::generate(Document &doc) const
{
    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        for (unsigned int instance = 0; instance < m_count; instance++) {
            if (it->get_type() == Entity::Type::LINE_2D) {
                const auto &li = dynamic_cast<const EntityLine2D &>(*it);
                if (li.m_wrkpl != m_active_wrkpl)
                    continue;
                {
                    auto new_line_uu = get_entity_uuid(uu, instance);
                    auto &new_line = doc.get_or_add_entity<EntityLine2D>(new_line_uu);
                    new_line.m_p1 = transform(li.m_p1, instance);
                    new_line.m_p2 = transform(li.m_p2, instance);
                    new_line.m_kind = ItemKind::GENRERATED;
                    new_line.m_generated_from = uu;
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
                    new_circle.m_center = transform(circle.m_center, instance);
                    new_circle.m_radius = circle.m_radius;
                    new_circle.m_kind = ItemKind::GENRERATED;
                    new_circle.m_generated_from = uu;
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
                    new_arc.m_from = transform(arc.m_from, instance);
                    new_arc.m_to = transform(arc.m_to, instance);
                    new_arc.m_center = transform(arc.m_center, instance);
                    new_arc.m_kind = ItemKind::GENRERATED;
                    new_arc.m_generated_from = uu;
                    new_arc.m_group = m_uuid;
                    new_arc.m_wrkpl = arc.m_wrkpl;
                }
            }
            else if (it->get_type() == Entity::Type::LINE_3D) {
                const auto &li = dynamic_cast<const EntityLine3D &>(*it);
                {
                    auto new_line_uu = get_entity_uuid(uu, instance);
                    auto &new_line = doc.get_or_add_entity<EntityLine3D>(new_line_uu);
                    new_line.m_p1 = transform(doc, li.m_p1, instance);
                    new_line.m_p2 = transform(doc, li.m_p2, instance);
                    new_line.m_kind = ItemKind::GENRERATED;
                    new_line.m_generated_from = uu;
                    new_line.m_group = m_uuid;
                }
            }
            else if (it->get_type() == Entity::Type::CIRCLE_3D) {
                const auto &circle = dynamic_cast<const EntityCircle3D &>(*it);
                {
                    auto new_circle_uu = get_entity_uuid(uu, instance);
                    auto &new_circle = doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                    new_circle.m_center = transform(doc, circle.m_center, instance);
                    new_circle.m_radius = circle.m_radius;
                    new_circle.m_kind = ItemKind::GENRERATED;
                    new_circle.m_generated_from = uu;
                    new_circle.m_group = m_uuid;
                    new_circle.m_normal = circle.m_normal;
                }
            }
            else if (it->get_type() == Entity::Type::ARC_3D) {
                const auto &arc = dynamic_cast<const EntityArc3D &>(*it);
                {
                    auto new_arc_uu = get_entity_uuid(uu, instance);
                    auto &new_arc = doc.get_or_add_entity<EntityArc3D>(new_arc_uu);
                    new_arc.m_from = transform(doc, arc.m_from, instance);
                    new_arc.m_to = transform(doc, arc.m_to, instance);
                    new_arc.m_center = transform(doc, arc.m_center, instance);
                    new_arc.m_kind = ItemKind::GENRERATED;
                    new_arc.m_generated_from = uu;
                    new_arc.m_group = m_uuid;
                    new_arc.m_normal = arc.m_normal;
                }
            }
        }
    }
}

std::set<UUID> GroupArray::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    return r;
}

std::set<UUID> GroupArray::get_referenced_groups(const Document &doc) const
{
    auto r = Group::get_referenced_groups(doc);
    r.insert(m_source_group);
    return r;
}

std::set<UUID> GroupArray::get_required_entities(const Document &doc) const
{
    return {};
}

std::set<UUID> GroupArray::get_required_groups(const Document &doc) const
{
    return {m_source_group};
}

std::list<GroupStatusMessage> GroupArray::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_array_messages.begin(), m_array_messages.end());
    return msg;
}

} // namespace dune3d
