#include "constraint_line_points_perpendicular.hpp"
#include "nlohmann/json.hpp"
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

std::set<EntityAndPoint> ConstraintLinePointsPerpendicular::get_referenced_entities_and_points() const
{
    return {{m_line, 0}, m_point_line, m_point};
}

void ConstraintLinePointsPerpendicular::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintLinePointsPerpendicular::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_point_line == old_point) {
        m_point_line = new_point;
        return true;
    }
    else if (m_point == old_point) {
        m_point = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
