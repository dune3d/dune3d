#pragma once
#include "constraintt.hpp"
#include "constraint_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {

template <typename T> void ConstraintT<T>::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(static_cast<const T &>(*this));
}

template <typename T> std::unique_ptr<Constraint> ConstraintT<T>::clone() const
{
    return std::make_unique<T>(static_cast<const T &>(*this));
}


template <typename T>
bool ConstraintT<T>::replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point)
{
    return replace_constraint_points(static_cast<T &>(*this), old_point, new_point);
}

template <typename T> std::set<EntityAndPoint> ConstraintT<T>::get_referenced_entities_and_points() const
{
    return get_referenced_entities_and_points_from_constraint(static_cast<const T &>(*this));
}

} // namespace dune3d
