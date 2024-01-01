#include "constraint_lock_rotation.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintLockRotation::ConstraintLockRotation(const UUID &uu) : Constraint(uu)
{
}

ConstraintLockRotation::ConstraintLockRotation(const UUID &uu, const json &j)
    : Constraint(uu, j), m_entity(j.at("entity").get<UUID>())
{
}

json ConstraintLockRotation::serialize() const
{
    json j = Constraint::serialize();
    j["entity"] = m_entity;
    return j;
}

std::unique_ptr<Constraint> ConstraintLockRotation::clone() const
{
    return std::make_unique<ConstraintLockRotation>(*this);
}

std::set<EntityAndPoint> ConstraintLockRotation::get_referenced_entities_and_points() const
{
    return {{m_entity, 0}};
}

void ConstraintLockRotation::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
