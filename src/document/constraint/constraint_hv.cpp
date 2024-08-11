#include "constraint_hv.hpp"
#include "constraint_util.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintHV::ConstraintHV(const UUID &uu) : Constraint(uu)
{
}

ConstraintHV::ConstraintHV(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity1(j.at("entity1").get<EntityAndPoint>()),
      m_entity2(j.at("entity2").get<EntityAndPoint>()), m_wrkpl(j.at("wrkpl").get<UUID>())
{
}

json ConstraintHV::serialize() const
{
    json j = Constraint::serialize();
    j["entity1"] = m_entity1;
    j["entity2"] = m_entity2;
    j["wrkpl"] = m_wrkpl;
    return j;
}

std::unique_ptr<Constraint> ConstraintHorizontal::clone() const
{
    return std::make_unique<ConstraintHorizontal>(*this);
}

std::unique_ptr<Constraint> ConstraintVertical::clone() const
{
    return std::make_unique<ConstraintVertical>(*this);
}

std::set<EntityAndPoint> ConstraintHV::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(*this);
}

void ConstraintHV::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

bool ConstraintHV::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(*this, old_point, new_point);
}

} // namespace dune3d
