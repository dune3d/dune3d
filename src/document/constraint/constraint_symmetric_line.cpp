#include "constraint_symmetric_line.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintSymmetricLine::ConstraintSymmetricLine(const UUID &uu) : Base(uu)
{
}

ConstraintSymmetricLine::ConstraintSymmetricLine(const UUID &uu, const json &j)
    : Base(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()), m_entity2(j.at("entity2").get<EntityAndPoint>()),
      m_line(j.at("line").get<UUID>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintSymmetricLine::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    j["line"] = m_line;
    return j;
}

} // namespace dune3d
