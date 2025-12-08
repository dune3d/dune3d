#include "constraint_util.hpp"
#include "preferences/preferences.hpp"
#include <format>
#include <sstream>

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

std::string format_constraint_value(double datum, const std::string &suffix)
{
    const auto &prefs = Preferences::get().editor;
    const auto rounding = prefs.constraint_value_rounding;
    const auto show_trailing = prefs.constraint_show_trailing_zeros;

    if (show_trailing || rounding == 0) {
        return std::format("{:.{}f}{}", datum, rounding, suffix);
    }

    return std::format("{:.{}g}{}", datum, rounding, suffix);
}

} // namespace dune3d
