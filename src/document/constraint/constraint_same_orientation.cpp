#include "constraint_same_orientation.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintSameOrientation::ConstraintSameOrientation(const UUID &uu) : Base(uu)
{
}

ConstraintSameOrientation::ConstraintSameOrientation(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_val(j.at("val").get<double>())
{
}

json ConstraintSameOrientation::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["val"] = m_val;
    return j;
}

} // namespace dune3d
