#include "constraint_arc_line_tangent.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintArcLineTangent::ConstraintArcLineTangent(const UUID &uu) : Base(uu)
{
}

ConstraintArcLineTangent::ConstraintArcLineTangent(const UUID &uu, const json &j)
    : Base(uu, j), m_arc(j.at("arc").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>())
{
}

json ConstraintArcLineTangent::serialize() const
{
    json j = Constraint::serialize();
    j["arc"] = m_arc;
    j["line"] = m_line;
    return j;
}

} // namespace dune3d
