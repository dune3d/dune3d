#include "constraint_equal_length.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintEqualLength::ConstraintEqualLength(const UUID &uu) : Base(uu)
{
}

ConstraintEqualLength::ConstraintEqualLength(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<UUID>()), m_entity2(j.at("entity2").get<UUID>()),
      m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintEqualLength::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}


std::set<EntityAndPoint> ConstraintEqualLength::get_referenced_entities_and_points() const
{
    std::set<EntityAndPoint> r = {{m_entity1, 0}, {m_entity2, 0}};
    if (m_wrkpl)
        r.emplace(m_wrkpl, 0);
    return r;
}

} // namespace dune3d
