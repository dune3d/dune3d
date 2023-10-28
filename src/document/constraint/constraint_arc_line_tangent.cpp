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

std::set<UUID> ConstraintArcLineTangent::get_referenced_entities() const
{
    return {m_arc.entity, m_line};
}

void ConstraintArcLineTangent::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
