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

std::set<EntityAndPoint> ConstraintPointInWorkplane::get_referenced_entities_and_points() const
{
    return {m_point, {m_wrkpl, 0}};
}


bool ConstraintPointInWorkplane::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_point == old_point) {
        m_point = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
