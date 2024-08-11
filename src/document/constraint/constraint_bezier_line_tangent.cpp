#include "constraint_bezier_line_tangent.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintBezierLineTangent::ConstraintBezierLineTangent(const UUID &uu) : Base(uu)
{
}

ConstraintBezierLineTangent::ConstraintBezierLineTangent(const UUID &uu, const json &j)
    : Base(uu, j), m_bezier(j.at("bezier").get<EntityAndPoint>()), m_line(j.at("line").get<UUID>())
{
}

json ConstraintBezierLineTangent::serialize() const
{
    json j = Constraint::serialize();
    j["bezier"] = m_bezier;
    j["line"] = m_line;
    return j;
}

} // namespace dune3d
