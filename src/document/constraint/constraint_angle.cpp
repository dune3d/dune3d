#include "constraint_angle.hpp"
#include "constraint_util.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraint_visitor.hpp"
#include "document/document.hpp"
#include <format>

namespace dune3d {
ConstraintAngleBase::ConstraintAngleBase(const UUID &uu) : Constraint(uu)
{
}

ConstraintAngleBase::ConstraintAngleBase(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}


json ConstraintAngleBase::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::set<EntityAndPoint> ConstraintAngleBase::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(*this);
}

bool ConstraintAngleBase::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(*this, old_point, new_point);
}

void ConstraintLinesAngle::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

void ConstraintLinesPerpendicular::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

std::unique_ptr<Constraint> ConstraintLinesPerpendicular::clone() const
{
    return std::make_unique<ConstraintLinesPerpendicular>(*this);
}

std::unique_ptr<Constraint> ConstraintLinesAngle::clone() const
{
    return std::make_unique<ConstraintLinesAngle>(*this);
}

ConstraintLinesAngle::ConstraintLinesAngle(const UUID &uu, const json &j)
    : ConstraintAngleBase(uu, j), m_angle(j.at("angle").get<double>()), m_negative(j.at("negative").get<bool>()),
      m_offset(j.at("offset").get<glm::dvec3>()), m_measurement(j.value("measurement", false))
{
}

static std::pair<double, double> closest_point_between_lines(const glm::dvec3 &a0, const glm::dvec3 &da,
                                                             const glm::dvec3 &b0, const glm::dvec3 &db)
{
    // Make a semi-orthogonal coordinate system from those directions;
    // note that dna and dnb need not be perpendicular.
    const auto dn = glm::cross(da, db);  // normal to both
    const auto dna = glm::cross(dn, da); // normal to da
    const auto dnb = glm::cross(dn, db); // normal to db

    // At the intersection of the lines
    //    a0 + pa*da = b0 + pb*db (where pa, pb are scalar params)
    // So dot this equation against dna and dnb to get two equations
    // to solve for da and db
    auto tb = glm::dot(a0 - b0, dna) / glm::dot(db, dna);
    auto ta = -glm::dot(a0 - b0, dnb) / glm::dot(da, dnb);
    return {ta, tb};
}


ConstraintLinesAngle::Vectors ConstraintLinesAngle::get_vectors(const Document &doc) const
{
    const auto l1p1 = doc.get_point({m_entity1, 1});
    const auto l1p2 = doc.get_point({m_entity1, 2});
    const auto l1v = l1p2 - l1p1;
    const auto l2p1 = doc.get_point({m_entity2, 1});
    const auto l2p2 = doc.get_point({m_entity2, 2});
    const auto l2v = (l2p2 - l2p1) * (m_negative ? -1. : 1.);
    const auto n = glm::normalize(glm::cross(l1v, l2v));
    const auto u = glm::normalize(l1v);
    const auto v = glm::normalize(glm::cross(n, u));

    return {.l1p1 = l1p1, .l2p1 = l2p1, .l1v = l1v, .l2v = l2v, .n = n, .u = u, .v = v};
}


glm::dvec3 ConstraintLinesAngle::get_origin(const Document &doc) const
{
    auto vecs = get_vectors(doc);

    const auto [u1, u2] = closest_point_between_lines(vecs.l1p1, vecs.l1v, vecs.l2p1, vecs.l2v);
    const auto i1 = vecs.l1p1 + vecs.l1v * u1;
    const auto i2 = vecs.l2p1 + vecs.l2v * u2;

    auto is = (i1 + i2) / 2.;
    return is;
}

json ConstraintLinesAngle::serialize() const
{
    auto j = ConstraintAngleBase::serialize();
    j["angle"] = m_angle;
    j["offset"] = m_offset;
    j["negative"] = m_negative;
    j["measurement"] = m_measurement;
    return j;
}

double ConstraintLinesAngle::measure_datum(const Document &doc) const
{
    auto vs = get_vectors(doc);
    return glm::degrees(acos(glm::dot(glm::normalize(vs.l1v), glm::normalize(vs.l2v))));
}

double ConstraintLinesAngle::get_display_datum(const Document &doc) const
{
    if (m_measurement)
        return measure_datum(doc);
    else
        return m_angle;
}

std::string ConstraintLinesAngle::format_datum(double datum) const
{
    return format_constraint_value(datum, "°");
}

} // namespace dune3d
