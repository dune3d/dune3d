#pragma once
#include "constraintt.hpp"
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

} // namespace dune3d
