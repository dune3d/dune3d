#include "constraint_diameter_radius.hpp"
#include "constraint_util.hpp"
#include "nlohmann/json.hpp"
#include "document/document.hpp"
#include "document/entity/ientity_radius.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintDiameterRadius::ConstraintDiameterRadius(const UUID &uu) : Constraint(uu)
{
}

ConstraintDiameterRadius::ConstraintDiameterRadius(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity(j.at("entity").get<UUID>()), m_distance(j.at("distance").get<double>()),
      m_offset(j.at("offset").get<glm::dvec2>()), m_measurement(j.value("measurement", false))
{
}

json ConstraintDiameterRadius::serialize() const
{
    json j = Constraint::serialize();
    j["entity"] = m_entity;
    j["offset"] = m_offset;
    j["distance"] = m_distance;
    j["measurement"] = m_measurement;
    return j;
}

std::unique_ptr<Constraint> ConstraintDiameter::clone() const
{
    return std::make_unique<ConstraintDiameter>(*this);
}

std::unique_ptr<Constraint> ConstraintRadius::clone() const
{
    return std::make_unique<ConstraintRadius>(*this);
}

std::set<EntityAndPoint> ConstraintDiameterRadius::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(*this);
}

bool ConstraintDiameterRadius::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(*this, old_point, new_point);
}

void ConstraintDiameterRadius::measure(const Document &doc)
{
    m_distance = measure_distance(doc);
}

double ConstraintDiameter::measure_distance(const Document &doc) const
{
    return 2 * measure_radius(doc);
}

double ConstraintRadius::measure_distance(const Document &doc) const
{
    return measure_radius(doc);
}

double ConstraintDiameterRadius::measure_radius(const Document &doc) const
{
    auto &entity = doc.get_entity<IEntityRadius>(m_entity);
    return entity.get_radius();
}

void ConstraintDiameterRadius::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

glm::dvec3 ConstraintDiameterRadius::get_origin(const Document &doc) const
{
    auto &en = doc.get_entity(m_entity);
    auto &en_radius = dynamic_cast<const IEntityRadius &>(en);
    return glm::dvec3(en_radius.get_center(), 0);
}

const UUID &ConstraintDiameterRadius::get_workplane(const Document &doc) const
{
    auto &en = doc.get_entity(m_entity);
    return dynamic_cast<const IEntityInWorkplane &>(en).get_workplane();
}

double ConstraintDiameterRadius::get_display_distance(const Document &doc) const
{
    if (m_measurement)
        return measure_distance(doc);
    else
        return m_distance;
}

} // namespace dune3d
