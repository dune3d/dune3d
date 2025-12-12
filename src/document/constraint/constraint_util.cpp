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
    const auto trailing_zeros = prefs.constraint_trailing_zeros;

    std::string formatted_number;

    switch (trailing_zeros) {
    case EditorPreferences::TrailingZeros::ON:
        formatted_number = std::format("{:.{}f}", datum, rounding);
        break;

    case EditorPreferences::TrailingZeros::ONE_DECIMAL:
        if (datum == std::round(datum)) {
            formatted_number = std::format("{:.1f}", datum);
        }
        else {
            formatted_number = std::format("{:.{}f}", datum, rounding);
            if (rounding > 0) {
                auto pos = formatted_number.find_last_not_of('0');
                if (pos != std::string::npos && formatted_number[pos] == '.') {
                    formatted_number = formatted_number.substr(0, pos);
                }
                else if (pos != std::string::npos) {
                    formatted_number = formatted_number.substr(0, pos + 1);
                }
            }
        }
        break;

    case EditorPreferences::TrailingZeros::OFF:
        formatted_number = std::format("{:.{}f}", datum, rounding);
        if (rounding > 0) {
            auto pos = formatted_number.find_last_not_of('0');
            if (pos != std::string::npos && formatted_number[pos] == '.') {
                formatted_number = formatted_number.substr(0, pos);
            }
            else if (pos != std::string::npos) {
                formatted_number = formatted_number.substr(0, pos + 1);
            }
        }
        break;
    }

    return formatted_number + suffix;
}

} // namespace dune3d
