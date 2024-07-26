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


std::set<EntityAndPoint> ConstraintSymmetricLine::get_referenced_entities_and_points() const
{
    return {m_entity1, m_entity2, {m_line, 0}, {m_wrkpl, 0}};
}

bool ConstraintSymmetricLine::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    if (m_entity1 == old_point) {
        m_entity1 = new_point;
        return true;
    }
    else if (m_entity2 == old_point) {
        m_entity2 = new_point;
        return true;
    }
    return false;
}

} // namespace dune3d
