#pragma once
#include <set>

namespace dune3d {
template <typename T, typename... Args> bool set_contains(const std::set<T> &s, Args &&...args)
{
    return (s.contains(args) || ...);
}
template <typename T, typename... Args> bool any_of(T value, Args &&...args)
{
    // Recursively check if value is equal to any of the arguments
    return ((value == args) || ...);
}
} // namespace dune3d
