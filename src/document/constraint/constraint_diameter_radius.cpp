#include "constraint_diameter_radius.hpp"
#include "nlohmann/json.hpp"
#include "document/document.hpp"
#include "document/entity/ientity_radius.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/json_util.hpp"
#include "util/glm_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintDiameterRadius::ConstraintDiameterRadius(const UUID &uu) : Constraint(uu)
{
}

ConstraintDiameterRadius::ConstraintDiameterRadius(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity(j.at("entity").get<UUID>()), m_distance(j.at("distance").get<double>()),
      m_offset(j.at("offset").get<glm::dvec2>())
{
}

json ConstraintDiameterRadius::serialize() const
{
    json j = Constraint::serialize();
    j["entity"] = m_entity;
    j["offset"] = m_offset;
    j["distance"] = m_distance;
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

std::set<UUID> ConstraintDiameterRadius::get_referenced_entities() const
{
    return {m_entity};
}

void ConstraintRadius::measure(const Document &doc)
{
    m_distance = measure_radius(doc);
}

void ConstraintDiameter::measure(const Document &doc)
{
    m_distance = 2 * measure_radius(doc);
}

double ConstraintDiameterRadius::measure_radius(const Document &doc)
{
    auto &entity = doc.get_entity<IEntityRadius>(m_entity);
    return entity.get_radius();
}

void ConstraintDiameterRadius::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

glm::dvec2 ConstraintDiameterRadius::get_origin(const Document &doc) const
{
    auto &en = doc.get_entity(m_entity);
    auto &en_radius = dynamic_cast<const IEntityRadius &>(en);
    return en_radius.get_center();
}

} // namespace dune3d
