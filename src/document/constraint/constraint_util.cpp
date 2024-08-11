#include "constraint_util.hpp"

namespace dune3d {
namespace detail {

bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point, UUID &entity)
{
    if (old_point.entity == entity) {
        entity = new_point.entity;
        return true;
    }
    return false;
}

bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point, EntityAndPoint &enp)
{
    if (old_point == enp) {
        enp = new_point;
        return true;
    }
    return false;
}

void add_to_referenced_enps(std::set<EntityAndPoint> &enps, const EntityAndPoint &enp)
{
    if (enp.entity)
        enps.insert(enp);
}

void add_to_referenced_enps(std::set<EntityAndPoint> &enps, const UUID &entity)
{
    if (entity)
        enps.emplace(entity, 0);
}

} // namespace detail

} // namespace dune3d
