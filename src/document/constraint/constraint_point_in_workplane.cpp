#include "constraint_point_in_workplane.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintPointInWorkplane::ConstraintPointInWorkplane(const UUID &uu) : Base(uu)
{
}

ConstraintPointInWorkplane::ConstraintPointInWorkplane(const UUID &uu, const json &j)
    : Base(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintPointInWorkplane::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["wrkpl"] = m_wrkpl;
    return j;
}

} // namespace dune3d
