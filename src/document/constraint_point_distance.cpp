#include "constraint_point_distance.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "entity.hpp"
#include "entity_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointDistanceBase::ConstraintPointDistanceBase(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointDistanceBase::ConstraintPointDistanceBase(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()),
      m_entity2(j.at("entity2").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>()),
      m_distance(j.at("distance").get<double>()), m_offset(j.at("offset").get<glm::dvec3>())
{
}

json ConstraintPointDistanceBase::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    j["offset"] = m_offset;
    j["distance"] = m_distance;
    return j;
}

void ConstraintPointDistanceBase::flip()
{
    std::swap(m_entity1, m_entity2);
}

std::unique_ptr<Constraint> ConstraintPointDistance::clone() const
{
    return std::make_unique<ConstraintPointDistance>(*this);
}

glm::dvec3 ConstraintPointDistanceBase::get_distance_vector(const Document &doc) const
{
    auto p1 = doc.get_point(m_entity1);
    auto p2 = doc.get_point(m_entity2);
    if (m_wrkpl) {
        auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
        return glm::dvec3(wrkpl.project(p2) - wrkpl.project(p1), 0);
    }
    else {
        return p2 - p1;
    }
}

double ConstraintPointDistance::measure_distance(const Document &doc) const
{
    return glm::length(get_distance_vector(doc));
}

std::set<UUID> ConstraintPointDistanceBase::get_referenced_entities() const
{
    std::set<UUID> r = {m_entity1.entity, m_entity2.entity};
    if (m_wrkpl)
        r.insert(m_wrkpl);
    return r;
}

void ConstraintPointDistance::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
