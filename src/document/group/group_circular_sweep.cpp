#include "group_circular_sweep.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/util.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"

namespace dune3d {
GroupCircularSweep::GroupCircularSweep(const UUID &uu) : GroupSweep(uu)
{
}

GroupCircularSweep::GroupCircularSweep(const UUID &uu, const json &j)
    : GroupSweep(uu, j), m_normal(j.at("normal").get<UUID>()),
      m_origin(j.at("origin").get<UUID>(), j.at("origin_point").get<unsigned int>())
{
}

json GroupCircularSweep::serialize() const
{
    auto j = GroupSweep::serialize();
    j["normal"] = m_normal;
    j["origin"] = m_origin.entity;
    j["origin_point"] = m_origin.point;
    return j;
}

std::optional<glm::dvec3> GroupCircularSweep::get_direction(const Document &doc) const
{

    const auto &en_normal = doc.get_entity(m_normal);
    const auto en_type = en_normal.get_type();
    if (auto wrkpl = dynamic_cast<const EntityWorkplane *>(&en_normal)) {
        return wrkpl->get_normal_vector();
    }
    else if (en_type == Entity::Type::LINE_2D || en_type == Entity::Type::LINE_3D) {
        return glm::normalize(en_normal.get_point(2, doc) - en_normal.get_point(1, doc));
    }
    else {
        return {};
    }
}

std::set<UUID> GroupCircularSweep::get_referenced_entities(const Document &doc) const
{
    auto r = GroupSweep::get_referenced_entities(doc);
    r.insert(m_normal);
    r.insert(m_origin.entity);
    return r;
}

std::set<UUID> GroupCircularSweep::get_required_entities(const Document &doc) const
{
    auto r = GroupSweep::get_required_entities(doc);
    r.insert(m_normal);
    r.insert(m_origin.entity);
    return r;
}


} // namespace dune3d
