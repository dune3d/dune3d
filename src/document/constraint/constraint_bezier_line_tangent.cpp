#include "constraint_bezier_line_tangent.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintBezierLineTangent::ConstraintBezierLineTangent(const UUID &uu) : Constraint(uu)
{
}

ConstraintBezierLineTangent::ConstraintBezierLineTangent(const UUID &uu, const json &j)
    : Constraint(uu, j), m_bezier(j.at("bezier").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>())
{
}

json ConstraintBezierLineTangent::serialize() const
{
    json j = Constraint::serialize();
    j["bezier"] = m_bezier;
    j["line"] = m_line;
    return j;
}

std::unique_ptr<Constraint> ConstraintBezierLineTangent::clone() const
{
    return std::make_unique<ConstraintBezierLineTangent>(*this);
}

std::set<EntityAndPoint> ConstraintBezierLineTangent::get_referenced_entities_and_points() const
{
    return {m_bezier, {m_line, 0}};
}

void ConstraintBezierLineTangent::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintBezierLineTangent::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_bezier == old_point) {
        m_bezier = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
