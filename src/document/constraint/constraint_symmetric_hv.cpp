#include "constraint_symmetric_hv.hpp"
#include "constraint_util.hpp"
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

void ConstraintSymmetricHV::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

std::set<EntityAndPoint> ConstraintSymmetricHV::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(*this);
}

bool ConstraintSymmetricHV::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(*this, old_point, new_point);
}

} // namespace dune3d
