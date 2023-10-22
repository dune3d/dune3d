#include "constraint_point_on_circle.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointOnCircle::ConstraintPointOnCircle(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointOnCircle::ConstraintPointOnCircle(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_circle(j.at("circle").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}


json ConstraintPointOnCircle::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["circle"] = m_circle;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintPointOnCircle::clone() const
{
    return std::make_unique<ConstraintPointOnCircle>(*this);
}

std::set<UUID> ConstraintPointOnCircle::get_referenced_entities() const
{
    return {m_point.entity, m_circle};
}

void ConstraintPointOnCircle::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
