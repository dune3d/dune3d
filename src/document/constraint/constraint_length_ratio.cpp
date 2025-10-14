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
#include <algorithm>
#include <format>
#include "util/glm_util.hpp"

namespace dune3d {

static glm::dvec3 get_entity_anchor(const Document &doc, const Entity &entity)
{
    if (auto arc2d = dynamic_cast<const EntityArc2D *>(&entity)) {
        return arc2d->get_point(3, doc);
    }
    if (auto arc3d = dynamic_cast<const EntityArc3D *>(&entity)) {
        return arc3d->get_point(3, doc);
    }
    if (entity.of_type(Entity::Type::CIRCLE_2D, Entity::Type::CIRCLE_3D)) {
        return entity.get_point(1, doc);
    }
    return (entity.get_point(1, doc) + entity.get_point(2, doc)) / 2.0;
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

double ConstraintLengthRatio::get_display_datum(const Document &) const
{
    return m_ratio;
}

double ConstraintLengthRatio::measure_datum(const Document &) const
{
    return m_ratio;
}

DatumUnit ConstraintLengthRatio::get_datum_unit() const
{
    return DatumUnit::RATIO;
}

std::string ConstraintLengthRatio::format_datum(double datum) const
{
    return std::format("{:.4f}×", datum);
}

} // namespace dune3d
