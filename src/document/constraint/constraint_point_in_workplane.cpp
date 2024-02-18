#include "constraint_point_in_workplane.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointInWorkplane::ConstraintPointInWorkplane(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointInWorkplane::ConstraintPointInWorkplane(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintPointInWorkplane::serialize() const
{
    json j = Constraint::serialize();
    j["point"] = m_point;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintPointInWorkplane::clone() const
{
    return std::make_unique<ConstraintPointInWorkplane>(*this);
}

std::set<EntityAndPoint> ConstraintPointInWorkplane::get_referenced_entities_and_points() const
{
    return {m_point, {m_wrkpl, 0}};
}

void ConstraintPointInWorkplane::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
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
