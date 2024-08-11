#include "constraint_point_on_line_base.hpp"
#include "constraint_util.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"

namespace dune3d {
ConstraintPointOnLineBase::ConstraintPointOnLineBase(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointOnLineBase::ConstraintPointOnLineBase(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintPointOnLineBase::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["line"] = m_line;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::set<EntityAndPoint> ConstraintPointOnLineBase::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(*this);
}

bool ConstraintPointOnLineBase::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(*this, old_point, new_point);
}

} // namespace dune3d
