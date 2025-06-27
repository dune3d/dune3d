#include "constraint_point_plane_distance.hpp"
#include "nlohmann/json.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintPointPlaneDistance::ConstraintPointPlaneDistance(const UUID &uu) : Base(uu)
{
}

ConstraintPointPlaneDistance::ConstraintPointPlaneDistance(const UUID &uu, const json &j)
    : Base(uu, j), m_distance(j.at("distance").get<double>()), m_offset(j.at("offset").get<glm::dvec3>()),
      m_measurement(j.value("measurement", false)), m_point(j.at("point").get<EntityAndPoint>()),
      m_line1(j.at("line1").get<UUID>()), m_line2(j.at("line2").get<UUID>())
{
}

json ConstraintPointPlaneDistance::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line1"] = m_line1;
    j["line2"] = m_line2;
    j["offset"] = m_offset;
    j["measurement"] = m_measurement;
    j["distance"] = m_distance;
    return j;
}

glm::dvec3 ConstraintPointPlaneDistance::get_origin(const Document &doc) const
{
    const auto pp = doc.get_point(m_point);
    const auto pproj = get_projected(doc);
    auto p = (pproj + pp) / 2.;
    return p;
}

glm::dvec3 ConstraintPointPlaneDistance::get_projected(const Document &doc) const
{
    auto &l1 = doc.get_entity(m_line1);
    auto &l2 = doc.get_entity(m_line2);
    const auto pp = doc.get_point(m_point);
    const auto p0 = l1.get_point(1, doc);
    const auto v1 = l1.get_point(2, doc) - l1.get_point(1, doc);
    const auto v2 = l2.get_point(2, doc) - l2.get_point(1, doc);
    const auto vn = glm::cross(v1, v2);
    const auto vu = glm::normalize(v1);
    const auto vv = glm::normalize(glm::cross(vn, vu));
    const auto vd = pp - p0;
    const auto tu = glm::dot(vu, vd);
    const auto tv = glm::dot(vv, vd);
    return p0 + tu * vu + tv * vv;
}

double ConstraintPointPlaneDistance::measure_distance(const Document &doc) const
{
    auto &l1 = doc.get_entity(m_line1);
    auto &l2 = doc.get_entity(m_line2);
    const auto pp = doc.get_point(m_point);
    const auto p0 = l1.get_point(1, doc);
    const auto v1 = l1.get_point(2, doc) - l1.get_point(1, doc);
    const auto v2 = l2.get_point(2, doc) - l2.get_point(1, doc);
    const auto vn = glm::normalize(glm::cross(v1, v2));
    const auto vd = pp - p0;
    return glm::dot(vn, vd);
}

double ConstraintPointPlaneDistance::measure_datum(const Document &doc) const
{
    return measure_distance(doc);
}

double ConstraintPointPlaneDistance::get_display_distance(const Document &doc) const
{
    if (m_measurement)
        return measure_distance(doc);
    else
        return m_distance;
}

} // namespace dune3d
