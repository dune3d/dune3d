#include "constraint_point_on_line.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointOnLine::ConstraintPointOnLine(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointOnLine::ConstraintPointOnLine(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>()), m_val(j.at("val").get<double>())
{
}


json ConstraintPointOnLine::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line"] = m_line;
    j["wrkpl"] = m_wrkpl;
    j["val"] = m_val;
    return j;
}

std::unique_ptr<Constraint> ConstraintPointOnLine::clone() const
{
    return std::make_unique<ConstraintPointOnLine>(*this);
}

std::set<UUID> ConstraintPointOnLine::get_referenced_entities() const
{
    return {m_point.entity, m_line};
}

void ConstraintPointOnLine::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
