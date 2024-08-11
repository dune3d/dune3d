#include "constraint_lock_rotation.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintLockRotation::ConstraintLockRotation(const UUID &uu) : Base(uu)
{
}

ConstraintLockRotation::ConstraintLockRotation(const UUID &uu, const json &j)
    : Base(uu, j), m_entity(j.at("entity").get<UUID>())
{
}

json ConstraintLockRotation::serialize() const
{
    json j = Constraint::serialize();
    j["entity"] = m_entity;
    return j;
}

} // namespace dune3d
