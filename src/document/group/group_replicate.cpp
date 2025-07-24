#include "group_replicate.hpp"
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
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_bezier3d.hpp"
#include "document/solid_model/solid_model.hpp"
#include "igroup_solid_model_json.hpp"

namespace dune3d {
GroupReplicate::GroupReplicate(const UUID &uu) : Group(uu)
{
}


NLOHMANN_JSON_SERIALIZE_ENUM(GroupReplicate::Sources, {
                                                              {GroupReplicate::Sources::BODY, "body"},
                                                              {GroupReplicate::Sources::RANGE, "range"},
                                                              {GroupReplicate::Sources::SINGLE, "single"},
                                                      })

GroupReplicate::GroupReplicate(const UUID &uu, const json &j)
    : Group(uu, j), m_source_group(j.at("source_group").get<UUID>()),
      m_sources(j.value("sources", GroupReplicate::Sources::SINGLE)),
      m_operation(j.value("operation", IGroupSolidModel::Operation::DIFFERENCE))
{
    if (m_sources == Sources::RANGE)
        m_source_group_start = j.at("source_group_start").get<UUID>();
}

json GroupReplicate::serialize() const
{
    auto j = Group::serialize();
    j["source_group"] = m_source_group;
    j["sources"] = m_sources;
    if (m_sources == Sources::RANGE)
        j["source_group_start"] = m_source_group_start;
    if (m_sources != Sources::SINGLE)
        j["operation"] = m_operation;
    return j;
}

const SolidModel *GroupReplicate::get_solid_model() const
{
    return m_solid_model.get();
}

UUID GroupReplicate::get_entity_uuid(const UUID &uu, unsigned int instance) const
{
    return hash_uuids("dee4fd38-6aa6-414f-bd45-524cf97b860b", {m_uuid, uu},
                      {reinterpret_cast<const uint8_t *>(&instance), sizeof(instance)});
}

bool GroupReplicate::is_source_group(const Document &doc, const UUID &uu) const
{
    return get_source_groups(doc).contains(uu);
}

std::vector<const Group *> GroupReplicate::get_source_groups_sorted(const Document &doc) const
{
    if (m_sources == Sources::SINGLE) {
        return {&doc.get_group(m_source_group)};
    }
    UUID start_group = m_source_group_start;

    if (m_sources == Sources::BODY) {
        auto &group = doc.get_group(m_source_group);
        start_group = group.find_body(doc).group.m_uuid;
    }

    std::vector<const Group *> r;
    bool have_start = false;
    for (auto group : doc.get_groups_sorted()) {
        if (group->m_uuid == start_group)
            have_start = true;
        if (have_start)
            r.push_back(group);
        if (group->m_uuid == m_source_group)
            return r;
    }
    return r;
}

std::set<UUID> GroupReplicate::get_source_groups(const Document &doc) const
{
    std::set<UUID> r;
    for (const auto group : get_source_groups_sorted(doc)) {
        r.insert(group->m_uuid);
    }
    return r;
}

void GroupReplicate::generate(Document &doc)
{
    for (const auto &[uu, it] : doc.m_entities) {
        if (!is_source_group(doc, it->m_group))
            continue;
        if (it->m_construction)
            continue;
        for (unsigned int instance = 0; instance < get_count(); instance++) {
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
                    post_add(new_line, *it, instance);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_2D) {
                const auto &bez = dynamic_cast<const EntityBezier2D &>(*it);
                if (bez.m_wrkpl != m_active_wrkpl)
                    continue;
                {
                    auto new_bez_uu = get_entity_uuid(uu, instance);
                    auto &new_line = doc.get_or_add_entity<EntityBezier2D>(new_bez_uu);
                    new_line.m_p1 = transform(bez.m_p1, instance);
                    new_line.m_p2 = transform(bez.m_p2, instance);
                    new_line.m_c1 = transform(bez.m_c1, instance);
                    new_line.m_c2 = transform(bez.m_c2, instance);
                    new_line.m_kind = ItemKind::GENRERATED;
                    new_line.m_generated_from = uu;
                    new_line.m_group = m_uuid;
                    new_line.m_wrkpl = bez.m_wrkpl;
                    post_add(new_line, *it, instance);
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
                    post_add(new_circle, *it, instance);
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
                    if (get_mirror_arc(instance)) {
                        new_arc.m_from = transform(arc.m_to, instance);
                        new_arc.m_to = transform(arc.m_from, instance);
                    }
                    else {
                        new_arc.m_from = transform(arc.m_from, instance);
                        new_arc.m_to = transform(arc.m_to, instance);
                    }
                    new_arc.m_center = transform(arc.m_center, instance);
                    new_arc.m_kind = ItemKind::GENRERATED;
                    new_arc.m_generated_from = uu;
                    new_arc.m_group = m_uuid;
                    new_arc.m_wrkpl = arc.m_wrkpl;
                    post_add(new_arc, *it, instance);
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
                    post_add(new_line, *it, instance);
                }
            }
            else if (it->get_type() == Entity::Type::BEZIER_3D) {
                const auto &bez = dynamic_cast<const EntityBezier3D &>(*it);
                {
                    auto new_bez_uu = get_entity_uuid(uu, instance);
                    auto &new_bez = doc.get_or_add_entity<EntityBezier3D>(new_bez_uu);
                    new_bez.m_p1 = transform(doc, bez.m_p1, instance);
                    new_bez.m_p2 = transform(doc, bez.m_p2, instance);
                    new_bez.m_c1 = transform(doc, bez.m_c1, instance);
                    new_bez.m_c2 = transform(doc, bez.m_c2, instance);
                    new_bez.m_kind = ItemKind::GENRERATED;
                    new_bez.m_generated_from = uu;
                    new_bez.m_group = m_uuid;
                    post_add(new_bez, *it, instance);
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
                    new_circle.m_normal = transform_normal(doc, circle.m_normal, instance);
                    post_add(new_circle, *it, instance);
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
                    new_arc.m_normal = transform_normal(doc, arc.m_normal, instance);
                    post_add(new_arc, *it, instance);
                }
            }
        }
    }
}

std::set<UUID> GroupReplicate::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    return r;
}

std::set<UUID> GroupReplicate::get_referenced_groups(const Document &doc) const
{
    auto r = Group::get_referenced_groups(doc);
    r.insert(m_source_group);
    return r;
}

std::set<UUID> GroupReplicate::get_required_entities(const Document &doc) const
{
    return {};
}

std::set<UUID> GroupReplicate::get_required_groups(const Document &doc) const
{
    return {m_source_group};
}

std::list<GroupStatusMessage> GroupReplicate::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_array_messages.begin(), m_array_messages.end());
    return msg;
}

glm::dquat GroupReplicate::transform_normal(const Document &doc, const glm::dquat &q, unsigned int instance) const
{
    return q;
}

void GroupReplicate::post_add(Entity &new_entity, const Entity &entity, unsigned int instance) const
{
}


} // namespace dune3d
