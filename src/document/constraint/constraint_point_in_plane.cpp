#include "constraint_point_in_plane.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointInPlane::ConstraintPointInPlane(const UUID &uu) : Constraint(uu)
{
}

ConstraintPointInPlane::ConstraintPointInPlane(const UUID &uu, const json &j)
    : Constraint(uu, j), m_point(j.at("point").get<EntityAndPoint>()), m_line1(j.at("line1").get<UUID>()),
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

std::unique_ptr<Constraint> ConstraintPointInPlane::clone() const
{
    return std::make_unique<ConstraintPointInPlane>(*this);
}

std::set<UUID> ConstraintPointInPlane::get_referenced_entities() const
{
    return {m_point.entity, m_line1, m_line2};
}

void ConstraintPointInPlane::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
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
