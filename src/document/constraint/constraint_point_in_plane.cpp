#include "constraint_point_in_plane.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintPointInPlane::ConstraintPointInPlane(const UUID &uu) : Base(uu)
{
}

ConstraintPointInPlane::ConstraintPointInPlane(const UUID &uu, const json &j)
    : Base(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line1(j.at("line1").get<UUID>()),
      m_line2(j.at("line2").get<UUID>())
{
}

json ConstraintPointInPlane::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line1"] = m_line1;
    j["line2"] = m_line2;
    return j;
}

std::set<EntityAndPoint> ConstraintPointInPlane::get_referenced_entities_and_points() const
{
    return {m_point, {m_line1, 0}, {m_line2, 0}};
}

bool ConstraintPointInPlane::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_point == old_point) {
        m_point = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
