#include "constraint_length_ratio.hpp"

#include "constraintt_impl.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_arc3d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "util/arc_util.hpp"
#include "util/glm_util.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

static constexpr double kLengthEpsilon = 1e-9;

static glm::dvec3 get_entity_anchor(const Document &doc, const Entity &entity)
{
    if (auto arc2d = dynamic_cast<const EntityArc2D *>(&entity)) {
        return arc2d->get_point(3, doc);
    }
    if (auto arc3d = dynamic_cast<const EntityArc3D *>(&entity)) {
        return arc3d->get_point(3, doc);
    }
    if (entity.get_type() == Entity::Type::CIRCLE_2D || entity.get_type() == Entity::Type::CIRCLE_3D) {
        return entity.get_point(1, doc);
    }
    return (entity.get_point(1, doc) + entity.get_point(2, doc)) / 2.0;
}

static double length_of_arc_2d(const EntityArc2D &arc)
{
    const auto from = arc.m_from - arc.m_center;
    const auto to = arc.m_to - arc.m_center;
    const auto radius = glm::length(from);
    if (radius <= kLengthEpsilon)
        return 0.0;

    const auto start = angle(from);
    const auto end = angle(to);
    const auto delta = c2pi(end - start);
    return radius * delta;
}

static double length_of_arc_3d(const EntityArc3D &arc)
{
    const auto from = arc.m_from - arc.m_center;
    const auto to = arc.m_to - arc.m_center;
    const auto radius = glm::length(from);
    if (radius <= kLengthEpsilon)
        return 0.0;

    const auto u = glm::rotate(arc.m_normal, glm::dvec3(1, 0, 0));
    const auto v = glm::rotate(arc.m_normal, glm::dvec3(0, 1, 0));

    const auto start = std::atan2(glm::dot(from, v), glm::dot(from, u));
    const auto end = std::atan2(glm::dot(to, v), glm::dot(to, u));
    const auto delta = c2pi(end - start);
    return radius * delta;
}

static double length_of_entity_impl(const Document &doc, const UUID &entity_uu)
{
    const auto &entity = doc.get_entity(entity_uu);
    switch (entity.get_type()) {
    case Entity::Type::LINE_2D:
    case Entity::Type::LINE_3D:
        return glm::length(doc.get_point({entity_uu, 2}) - doc.get_point({entity_uu, 1}));

    case Entity::Type::ARC_2D:
        return length_of_arc_2d(dynamic_cast<const EntityArc2D &>(entity));

    case Entity::Type::ARC_3D:
        return length_of_arc_3d(dynamic_cast<const EntityArc3D &>(entity));

    default:
        return 0.0;
    }
}

ConstraintLengthRatio::ConstraintLengthRatio(const UUID &uu) : Base(uu)
{
}

ConstraintLengthRatio::ConstraintLengthRatio(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_ratio(j.value("ratio", 1.0)), m_wrkpl(j.value("wrkpl", UUID{})), m_measurement(j.value("measurement", false))
{
    m_offset = j.value("offset", glm::dvec3(0.0, 0.0, 0.0));
    m_ratio = std::clamp(m_ratio, s_min_ratio, s_max_ratio);
}

json ConstraintLengthRatio::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["ratio"] = m_ratio;
    j["wrkpl"] = m_wrkpl;
    j["measurement"] = m_measurement;
    j["offset"] = m_offset;
    return j;
}

void ConstraintLengthRatio::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

glm::dvec3 ConstraintLengthRatio::get_origin(const Document &doc) const
{
    const auto &entity1 = doc.get_entity(m_entity1);
    const auto &entity2 = doc.get_entity(m_entity2);
    return (get_entity_anchor(doc, entity1) + get_entity_anchor(doc, entity2)) / 2.0;
}

double ConstraintLengthRatio::get_display_datum(const Document &doc) const
{
    if (m_measurement)
        return measure_ratio(doc);
    return m_ratio;
}

double ConstraintLengthRatio::measure_datum(const Document &doc) const
{
    return measure_ratio(doc);
}

DatumUnit ConstraintLengthRatio::get_datum_unit() const
{
    return DatumUnit::RATIO;
}

std::string ConstraintLengthRatio::format_datum(double datum) const
{
    return std::format("{:.4f}", datum);
}

double ConstraintLengthRatio::measure_entity_length(const Document &doc, const UUID &entity)
{
    return length_of_entity_impl(doc, entity);
}

double ConstraintLengthRatio::measure_ratio(const Document &doc) const
{
    const auto len2 = length_of_entity_impl(doc, m_entity2);
    const auto len1 = length_of_entity_impl(doc, m_entity1);

    double ratio = s_min_ratio;
    if (len2 > kLengthEpsilon)
        ratio = len1 / len2;
    else if (len1 <= kLengthEpsilon)
        ratio = 1.0;
    else
        ratio = s_max_ratio;

    return std::clamp(ratio, s_min_ratio, s_max_ratio);
}

} // namespace dune3d
