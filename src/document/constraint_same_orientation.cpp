#include "constraint_same_orientation.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintSameOrientation::ConstraintSameOrientation(const UUID &uu) : Constraint(uu)
{
}

ConstraintSameOrientation::ConstraintSameOrientation(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_val(j.at("val").get<double>())
{
}


json ConstraintSameOrientation::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["val"] = m_val;
    return j;
}

std::unique_ptr<Constraint> ConstraintSameOrientation::clone() const
{
    return std::make_unique<ConstraintSameOrientation>(*this);
}

std::set<UUID> ConstraintSameOrientation::get_referenced_entities() const
{
    return {m_entity1, m_entity2};
}

void ConstraintSameOrientation::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
