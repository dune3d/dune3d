#include "constraint_equal_radius.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintEqualRadius::ConstraintEqualRadius(const UUID &uu) : Base(uu)
{
}

ConstraintEqualRadius::ConstraintEqualRadius(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>())
{
}

json ConstraintEqualRadius::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    return j;
}

} // namespace dune3d
