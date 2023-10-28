#include "group_lathe.hpp"
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
GroupLathe::GroupLathe(const UUID &uu) : GroupSweep(uu)
{
}


GroupLathe::GroupLathe(const UUID &uu, const json &j)
    : GroupSweep(uu, j), m_normal(j.at("normal").get<UUID>()), m_origin(j.at("origin").get<UUID>()),
      m_origin_point(j.at("origin_point").get<unsigned int>())
{
}

json GroupLathe::serialize() const
{
    auto j = GroupSweep::serialize();
    j["normal"] = m_normal;
    j["origin"] = m_origin;
    j["origin_point"] = m_origin_point;
    return j;
}

std::unique_ptr<Group> GroupLathe::clone() const
{
    return std::make_unique<GroupLathe>(*this);
}


UUID GroupLathe::get_lathe_circle_uuid(const UUID &uu, unsigned int pt) const
{
    return hash_uuids("e47e6775-3fe4-473a-84f8-c2c5578a10af", {m_uuid, m_wrkpl, uu},
                      {reinterpret_cast<const uint8_t *>(&pt), sizeof(pt)});
}


void GroupLathe::generate(Document &doc) const
{
    // TBD
    /*
    auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);

    for (const auto &[uu, it] : doc.m_entities) {
        if (it->m_group != m_source_group)
            continue;
        if (it->m_construction)
            continue;
        if (it->get_type() == Entity::Type::LINE_2D) {
            const auto &li = dynamic_cast<const EntityLine2D &>(*it);
            if (li.m_wrkpl != m_wrkpl)
                continue;


            for (unsigned int pt = 1; pt <= 2; pt++) {
                auto new_circle_uu = get_lathe_circle_uuid(uu, pt);

                auto &new_circle = doc.get_or_add_entity<EntityCircle3D>(new_circle_uu);
                // new_circle.m_normal =
                // new_circle.new_line.m_p1 = wrkpl.transform(li.get_point(pt, doc));
                // new_line.m_p2 = wrkpl.transform(li.get_point(pt, doc)) + dvec;
                new_circle.m_group = m_uuid;
                // new_line.m_name = "Extrusion" + std::to_string(pt);
                //   new_line.m_wrkpl = new_wrkpl_uu;

                new_circle.m_kind = ItemKind::GENRERATED;
            }
        }
    }
    */
}

void GroupLathe::update_solid_model(const Document &doc)
{
    m_solid_model = SolidModel::create(doc, *this);
}

std::set<UUID> GroupLathe::get_referenced_entities(const Document &doc) const
{
    auto r = GroupSweep::get_referenced_entities(doc);
    r.insert(m_normal);
    r.insert(m_origin);
    return r;
}

std::set<UUID> GroupLathe::get_required_entities(const Document &doc) const
{
    auto r = GroupSweep::get_required_entities(doc);
    r.insert(m_normal);
    r.insert(m_origin);
    return r;
}


} // namespace dune3d
