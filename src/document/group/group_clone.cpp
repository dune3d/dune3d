#include "group_clone.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_circle2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_bezier2d.hpp"

namespace dune3d {
GroupClone::GroupClone(const UUID &uu) : Group(uu)
{
}


GroupClone::GroupClone(const UUID &uu, const json &j)
    : Group(uu, j), m_source_wrkpl(j.at("source_wrkpl").get<UUID>()), m_source_group(j.at("source_group").get<UUID>())
{
}

json GroupClone::serialize() const
{
    auto j = Group::serialize();
    j["source_group"] = m_source_group;
    j["source_wrkpl"] = m_source_wrkpl;
    return j;
}

UUID GroupClone::get_entity_uuid(const UUID &uu) const
{
    return hash_uuids("35875a02-f865-4617-ae24-4f5ecdf44975", {m_uuid, uu});
}

void GroupClone::generate(Document &doc)
{
    m_clone_messages.clear();
    if (!m_active_wrkpl) {
        m_clone_messages.emplace_back(GroupStatusMessage::Status::ERR, "Group needs workplane");
        return;
    }

    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->get_type() == Entity::Type::LINE_2D) {
            const auto &li = dynamic_cast<const EntityLine2D &>(*it);
            if (li.m_wrkpl != m_source_wrkpl)
                continue;
            {
                auto new_line_uu = get_entity_uuid(uu);
                auto &new_line = doc.get_or_add_entity<EntityLine2D>(new_line_uu);
                new_line.m_p1 = li.m_p1;
                new_line.m_p2 = li.m_p2;
                new_line.m_kind = ItemKind::GENRERATED;
                new_line.m_generated_from = uu;
                new_line.m_group = m_uuid;
                new_line.m_wrkpl = m_active_wrkpl;
                new_line.m_construction = li.m_construction;
            }
        }
        else if (it->get_type() == Entity::Type::BEZIER_2D) {
            const auto &bez = dynamic_cast<const EntityBezier2D &>(*it);
            if (bez.m_wrkpl != m_source_wrkpl)
                continue;
            {
                auto new_bez_uu = get_entity_uuid(uu);
                auto &new_line = doc.get_or_add_entity<EntityBezier2D>(new_bez_uu);
                new_line.m_p1 = bez.m_p1;
                new_line.m_p2 = bez.m_p2;
                new_line.m_c1 = bez.m_c1;
                new_line.m_c2 = bez.m_c2;
                new_line.m_kind = ItemKind::GENRERATED;
                new_line.m_generated_from = uu;
                new_line.m_group = m_uuid;
                new_line.m_wrkpl = m_active_wrkpl;
                new_line.m_construction = bez.m_construction;
            }
        }
        else if (it->get_type() == Entity::Type::CIRCLE_2D) {
            const auto &circle = dynamic_cast<const EntityCircle2D &>(*it);
            if (circle.m_wrkpl != m_source_wrkpl)
                continue;
            {
                auto new_circle_uu = get_entity_uuid(uu);
                auto &new_circle = doc.get_or_add_entity<EntityCircle2D>(new_circle_uu);
                new_circle.m_center = circle.m_center;
                new_circle.m_radius = circle.m_radius;
                new_circle.m_kind = ItemKind::GENRERATED;
                new_circle.m_generated_from = uu;
                new_circle.m_group = m_uuid;
                new_circle.m_wrkpl = m_active_wrkpl;
                new_circle.m_construction = circle.m_construction;
            }
        }
        else if (it->get_type() == Entity::Type::ARC_2D) {
            const auto &arc = dynamic_cast<const EntityArc2D &>(*it);
            if (arc.m_wrkpl != m_source_wrkpl)
                continue;
            {
                auto new_arc_uu = get_entity_uuid(uu);
                auto &new_arc = doc.get_or_add_entity<EntityArc2D>(new_arc_uu);
                new_arc.m_no_radius_constraint = true;
                new_arc.m_from = arc.m_from;
                new_arc.m_to = arc.m_to;
                new_arc.m_center = arc.m_center;
                new_arc.m_kind = ItemKind::GENRERATED;
                new_arc.m_generated_from = uu;
                new_arc.m_group = m_uuid;
                new_arc.m_wrkpl = m_active_wrkpl;
                new_arc.m_construction = arc.m_construction;
            }
        }
    }
}

std::set<UUID> GroupClone::get_referenced_entities(const Document &doc) const
{
    auto r = Group::get_referenced_entities(doc);
    r.insert(m_source_wrkpl);
    return r;
}

std::set<UUID> GroupClone::get_referenced_groups(const Document &doc) const
{
    auto r = Group::get_referenced_groups(doc);
    r.insert(m_source_group);
    return r;
}

std::set<UUID> GroupClone::get_required_entities(const Document &doc) const
{
    return {m_source_wrkpl};
}

std::set<UUID> GroupClone::get_required_groups(const Document &doc) const
{
    return {m_source_group};
}

std::list<GroupStatusMessage> GroupClone::get_messages() const
{
    auto msg = Group::get_messages();
    msg.insert(msg.end(), m_clone_messages.begin(), m_clone_messages.end());
    return msg;
}

std::unique_ptr<Group> GroupClone::clone() const
{
    return std::make_unique<GroupClone>(*this);
}


} // namespace dune3d
