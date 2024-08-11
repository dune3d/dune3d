#include "constraint_line_points_perpendicular.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintLinePointsPerpendicular::ConstraintLinePointsPerpendicular(const UUID &uu) : Base(uu)
{
}

ConstraintLinePointsPerpendicular::ConstraintLinePointsPerpendicular(const UUID &uu, const json &j)
    : Base(uu, j), m_line(j.at("line").get<UUID>()), m_point_line(j.at("point_line").get<EntityAndPoint>()),
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

} // namespace dune3d
