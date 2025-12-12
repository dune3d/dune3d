#pragma once
#include <set>
#include <string>
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class UUID;

namespace detail {
bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point, UUID &entity);
bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point, EntityAndPoint &enp);
} // namespace detail

template <typename... Args>
bool replace_points(const EntityAndPoint &old_point, const EntityAndPoint &new_point, Args &...items)
{
    // use | rather than || since we don't want short-circuit evaluation
    return (detail::replace_point(old_point, new_point, items) | ...);
}

namespace detail {

void add_to_referenced_enps(std::set<EntityAndPoint> &enps, const EntityAndPoint &enp);
void add_to_referenced_enps(std::set<EntityAndPoint> &enps, const UUID &entity);

} // namespace detail

template <typename... Args> std::set<EntityAndPoint> make_referenced_enps(const Args &...items)
{
    std::set<EntityAndPoint> enps;
    (detail::add_to_referenced_enps(enps, items), ...);
    return enps;
}

namespace detail {
template <class T, std::size_t... Is>
auto replace_constraint_points_helper(T &constraint, const EntityAndPoint &old_point, const EntityAndPoint &new_point,
                                      std::index_sequence<Is...>)
{
    return replace_points(old_point, new_point, constraint.*std::get<Is>(T::s_referenced_entities_and_points_tuple)...);
}
} // namespace detail

template <typename T>
bool replace_constraint_points(T &constraint, const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    constexpr auto size = std::tuple_size_v<decltype(T::s_referenced_entities_and_points_tuple)>;
    return detail::replace_constraint_points_helper(constraint, old_point, new_point, std::make_index_sequence<size>{});
}

namespace detail {
template <class T, std::size_t... Is>
auto get_referenced_entities_and_points_from_constraint_helper(const T &constraint, std::index_sequence<Is...>)
{
    return make_referenced_enps(constraint.*std::get<Is>(T::s_referenced_entities_and_points_tuple)...);
}
} // namespace detail

template <typename T> std::set<EntityAndPoint> get_referenced_entities_and_points_from_constraint(const T &constraint)
{
    constexpr auto size = std::tuple_size_v<decltype(T::s_referenced_entities_and_points_tuple)>;
    return detail::get_referenced_entities_and_points_from_constraint_helper(constraint,
                                                                             std::make_index_sequence<size>{});
}

std::string format_constraint_value(double datum, const std::string &suffix = "");

} // namespace dune3d
