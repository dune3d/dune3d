#include "constraint_arc_line_tangent.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintArcLineTangent::ConstraintArcLineTangent(const UUID &uu) : Constraint(uu)
{
}

ConstraintArcLineTangent::ConstraintArcLineTangent(const UUID &uu, const json &j)
    : Constraint(uu, j), m_arc(j.at("arc").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>())
{
}

json ConstraintArcLineTangent::serialize() const
{
    json j = Constraint::serialize();
    j["arc"] = m_arc;
    j["line"] = m_line;
    return j;
}

std::unique_ptr<Constraint> ConstraintArcLineTangent::clone() const
{
    return std::make_unique<ConstraintArcLineTangent>(*this);
}

std::set<EntityAndPoint> ConstraintArcLineTangent::get_referenced_entities_and_points() const
{
    return {m_arc, {m_line, 0}};
}

void ConstraintArcLineTangent::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintArcLineTangent::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_arc == old_point) {
        m_arc = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
