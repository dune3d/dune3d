#include "constraint_arc_arc_tangent.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintArcArcTangent::ConstraintArcArcTangent(const UUID &uu) : Constraint(uu)
{
}

ConstraintArcArcTangent::ConstraintArcArcTangent(const UUID &uu, const json &j)
    : Constraint(uu, j), m_arc1(j.at("arc1").get<EntityAndPoint>()), m_arc2(j.at("arc2").get<EntityAndPoint>())
{
}

json ConstraintArcArcTangent::serialize() const
{
    json j = Constraint::serialize();
    j["arc1"] = m_arc1;
    j["arc2"] = m_arc2;
    return j;
}

std::unique_ptr<Constraint> ConstraintArcArcTangent::clone() const
{
    return std::make_unique<ConstraintArcArcTangent>(*this);
}

std::set<EntityAndPoint> ConstraintArcArcTangent::get_referenced_entities_and_points() const
{
    return {m_arc1, m_arc2};
}

void ConstraintArcArcTangent::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintArcArcTangent::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_arc1 == old_point) {
        m_arc1 = new_point;
        return true;
    }
    else if (m_arc2 == old_point) {
        m_arc2 = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
