#include "constraint_points_coincident.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointsCoincident::ConstraintPointsCoincident(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointsCoincident::ConstraintPointsCoincident(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()),
      m_entity2(j.at("entity2").get<EntityAndPoint>())
{
    if (j.count("wrkpl"))
        m_wrkpl = j.at("wrkpl").get<UUID>();
}


json ConstraintPointsCoincident::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    if (m_wrkpl)
        j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintPointsCoincident::clone() const
{
    return std::make_unique<ConstraintPointsCoincident>(*this);
}

std::set<UUID> ConstraintPointsCoincident::get_referenced_entities() const
{
    return {m_entity1.entity, m_entity2.entity};
}

void ConstraintPointsCoincident::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
