#include "constraint_point_on_line_base.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

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
    return {m_point, {m_line, 0}};
}

bool ConstraintPointOnLineBase::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_point == old_point) {
        m_point = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
