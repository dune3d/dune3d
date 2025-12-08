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

    std::string result;

    if (show_trailing || rounding == 0) {
        result = std::format("{:.{}f}", datum, rounding);
    }
    else {
        std::ostringstream oss;
        oss << std::fixed;
        oss.precision(rounding);
        oss << datum;

        result = oss.str();

        size_t decimal_pos = result.find('.');
        if (decimal_pos != std::string::npos) {
            size_t last_non_zero = result.find_last_not_of('0');
            if (last_non_zero != std::string::npos) {
                if (last_non_zero == decimal_pos) {
                    result = result.substr(0, decimal_pos);
                }
                else {
                    result = result.substr(0, last_non_zero + 1);
                }
            }
        }
    }

    return result + suffix;
}

} // namespace dune3d
