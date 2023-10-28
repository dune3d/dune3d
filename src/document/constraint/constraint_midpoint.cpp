#include "constraint_midpoint.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintMidpoint::ConstraintMidpoint(const UUID &uu) : Constraint(uu)
{
}

ConstraintMidpoint::ConstraintMidpoint(const UUID &uu, const json &j)
    : Constraint(uu, j), m_line(j.at("line").get<UUID>()), m_point(j.at("point").get<EntityAndPoint>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintMidpoint::serialize() const
{
    json j = Constraint::serialize();
    j["line"] = m_line;
    j["point"] = m_point;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintMidpoint::clone() const
{
    return std::make_unique<ConstraintMidpoint>(*this);
}

std::set<UUID> ConstraintMidpoint::get_referenced_entities() const
{
    std::set<UUID> r = {m_point.entity, m_line};
    if (m_wrkpl)
        r.insert(m_wrkpl);
    return r;
}

void ConstraintMidpoint::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
