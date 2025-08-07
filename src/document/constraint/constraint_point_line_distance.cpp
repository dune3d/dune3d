#include "constraint_point_line_distance.hpp"
#include "nlohmann/json.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintPointLineDistance::ConstraintPointLineDistance(const UUID &uu) : Base(uu)
{
}

ConstraintPointLineDistance::ConstraintPointLineDistance(const UUID &uu, const json &j)
    : Base(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_distance(j.at("distance").get<double>()),
      m_offset(j.at("offset").get<glm::dvec3>()), m_measurement(j.value("measurement", false))
{
}

json ConstraintPointLineDistance::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line"] = m_line;
    j["wrkpl"] = m_wrkpl;
    j["offset"] = m_offset;
    j["measurement"] = m_measurement;
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

double ConstraintPointLineDistance::measure_distance(const Document &doc) const
{
    auto lp1 = doc.get_point({m_line, 1});
    auto lp2 = doc.get_point({m_line, 2});
    auto pp = doc.get_point(m_point);
    if (m_wrkpl) {
        auto &wrkpl = doc.get_entity<EntityWorkplane>(m_wrkpl);
        lp1 = wrkpl.project3(lp1);
        lp2 = wrkpl.project3(lp2);
        pp = wrkpl.project3(pp);
    }
    const auto v = glm::normalize(lp2 - lp1);
    const auto t = glm::dot(v, pp - lp1);
    return glm::length(pp - (lp1 + v * t));
}

double ConstraintPointLineDistance::measure_datum(const Document &doc) const
{
    return measure_distance(doc);
}

double ConstraintPointLineDistance::get_display_datum(const Document &doc) const
{
    if (m_measurement)
        return measure_distance(doc);
    else
        return m_distance;
}

} // namespace dune3d
