#include "constraint_symmetric_hv.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintSymmetricHV::ConstraintSymmetricHV(const UUID &uu) : Constraint(uu)
{
}

ConstraintSymmetricHV::ConstraintSymmetricHV(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()),
      m_entity2(j.at("entity2").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintSymmetricHV::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintSymmetricHorizontal::clone() const
{
    return std::make_unique<ConstraintSymmetricHorizontal>(*this);
}

std::unique_ptr<Constraint> ConstraintSymmetricVertical::clone() const
{
    return std::make_unique<ConstraintSymmetricVertical>(*this);
}

std::set<EntityAndPoint> ConstraintSymmetricHV::get_referenced_entities_and_points() const
{
    return {m_entity1, m_entity2, {m_wrkpl, 0}};
}

void ConstraintSymmetricHV::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintSymmetricHV::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
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
