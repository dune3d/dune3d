#include "constraint_point_on_circle.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintPointOnCircle::ConstraintPointOnCircle(const UUID &uu) : Base(uu)
{
}

ConstraintPointOnCircle::ConstraintPointOnCircle(const UUID &uu, const json &j)
    : Base(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_circle(j.at("circle").get<UUID>())
{
}

json ConstraintPointOnCircle::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["circle"] = m_circle;
    j["wrkpl"] = UUID(); // for backwards compatibility
    return j;
}


std::set<EntityAndPoint> ConstraintPointOnCircle::get_referenced_entities_and_points() const
{
    return {m_point, {m_circle, 0}};
}

bool ConstraintPointOnCircle::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_point == old_point) {
        m_point = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
