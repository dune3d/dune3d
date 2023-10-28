#include "constraint_hv.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintHV::ConstraintHV(const UUID &uu) : Constraint(uu)
{
}

ConstraintHV::ConstraintHV(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()),
      m_entity2(j.at("entity2").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintHV::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintHorizontal::clone() const
{
    return std::make_unique<ConstraintHorizontal>(*this);
}

std::unique_ptr<Constraint> ConstraintVertical::clone() const
{
    return std::make_unique<ConstraintVertical>(*this);
}

std::set<UUID> ConstraintHV::get_referenced_entities() const
{
    return {m_entity1.entity, m_entity2.entity, m_wrkpl};
}

void ConstraintHV::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
