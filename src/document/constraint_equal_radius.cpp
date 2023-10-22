#include "constraint_equal_radius.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintEqualRadius::ConstraintEqualRadius(const UUID &uu) : Constraint(uu)
{
}

ConstraintEqualRadius::ConstraintEqualRadius(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>())
{
}

json ConstraintEqualRadius::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    return j;
}

std::unique_ptr<Constraint> ConstraintEqualRadius::clone() const
{
    return std::make_unique<ConstraintEqualRadius>(*this);
}

std::set<UUID> ConstraintEqualRadius::get_referenced_entities() const
{
    return {m_entity1, m_entity2};
}

void ConstraintEqualRadius::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
