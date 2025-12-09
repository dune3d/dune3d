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

    switch (trailing_zeros) {
    case EditorPreferences::TrailingZeros::ON:
        return std::format("{:.{}f}{}", datum, rounding, suffix);

    case EditorPreferences::TrailingZeros::ONE_DECIMAL:
        if (datum == std::round(datum)) {
            return std::format("{:.1f}{}", datum, suffix);
        }
        [[fallthrough]];

    case EditorPreferences::TrailingZeros::OFF: {
        auto formatted = std::format("{:.{}f}{}", datum, rounding, suffix);
        if (rounding > 0) {
            auto pos = formatted.find_last_not_of('0');
            if (pos != std::string::npos && formatted[pos] == '.') {
                formatted = formatted.substr(0, pos);
            }
            else if (pos != std::string::npos) {
                formatted = formatted.substr(0, pos + 1);
            }
        }
        return formatted;
    }
    }

    auto formatted = std::format("{:.{}f}{}", datum, rounding, suffix);
    if (rounding > 0) {
        auto pos = formatted.find_last_not_of('0');
        if (pos != std::string::npos && formatted[pos] == '.') {
            formatted = formatted.substr(0, pos);
        }
        else if (pos != std::string::npos) {
            formatted = formatted.substr(0, pos + 1);
        }
    }
    return formatted;
}

} // namespace dune3d
