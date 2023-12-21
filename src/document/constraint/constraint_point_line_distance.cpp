#include "constraint_point_line_distance.hpp"
#include "nlohmann/json.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointLineDistance::ConstraintPointLineDistance(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointLineDistance::ConstraintPointLineDistance(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_distance(j.at("distance").get<double>()),
      m_offset(j.at("offset").get<glm::dvec3>())
{
}

json ConstraintPointLineDistance::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line"] = m_line;
    j["wrkpl"] = m_wrkpl;
    j["offset"] = m_offset;
    j["distance"] = m_distance;
    return j;
}

glm::dvec3 ConstraintPointLineDistance::get_projected(const Document &doc) const
{
    const auto lp1 = doc.get_point({m_line, 1});
    const auto lp2 = doc.get_point({m_line, 2});
    const auto lv = glm::normalize(lp2 - lp1);
    const auto pp = doc.get_point(m_point);
    const auto pv = pp - lp1;
    const auto pproj = lp1 + lv * glm::dot(lv, pv);
    return pproj;
}

glm::dvec3 ConstraintPointLineDistance::get_origin(const Document &doc) const
{
    const auto pp = doc.get_point(m_point);
    const auto pproj = get_projected(doc);
    auto p = (pproj + pp) / 2.;
    return p;
}


std::unique_ptr<Constraint> ConstraintPointLineDistance::clone() const
{
    return std::make_unique<ConstraintPointLineDistance>(*this);
}

std::set<UUID> ConstraintPointLineDistance::get_referenced_entities() const
{
    std::set<UUID> r = {m_point.entity, m_line};
    if (m_wrkpl)
        r.insert(m_wrkpl);
    return r;
}

void ConstraintPointLineDistance::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
