#include "constraint_line_points_perpendicular.hpp"
#include "nlohmann/json.hpp"
#include "document.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintLinePointsPerpendicular::ConstraintLinePointsPerpendicular(const UUID &uu) : Constraint(uu)
{
}

ConstraintLinePointsPerpendicular::ConstraintLinePointsPerpendicular(const UUID &uu, const json &j)
    : Constraint(uu, j), m_line(j.at("line").get<UUID>()), m_point_line(j.at("point_line").get<EntityAndPoint>()),
      m_point(j.at("point").get<EntityAndPoint>())
{
}

json ConstraintLinePointsPerpendicular::serialize() const
{
    json j = Constraint::serialize();
    j["line"] = m_line;
    j["point_line"] = m_point_line;
    j["point"] = m_point;

    return j;
}

std::unique_ptr<Constraint> ConstraintLinePointsPerpendicular::clone() const
{
    return std::make_unique<ConstraintLinePointsPerpendicular>(*this);
}

std::set<UUID> ConstraintLinePointsPerpendicular::get_referenced_entities() const
{
    return {m_line, m_point_line.entity, m_point.entity};
}

void ConstraintLinePointsPerpendicular::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
