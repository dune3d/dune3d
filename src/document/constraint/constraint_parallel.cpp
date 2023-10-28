#include "constraint_parallel.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintParallel::ConstraintParallel(const UUID &uu) : Constraint(uu)
{
}

ConstraintParallel::ConstraintParallel(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_val(j.at("val").get<double>())
{
}


json ConstraintParallel::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    j["val"] = m_val;
    return j;
}

std::unique_ptr<Constraint> ConstraintParallel::clone() const
{
    return std::make_unique<ConstraintParallel>(*this);
}

std::set<UUID> ConstraintParallel::get_referenced_entities() const
{
    return {m_entity1, m_entity2};
}

void ConstraintParallel::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
