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

} // namespace dune3d
