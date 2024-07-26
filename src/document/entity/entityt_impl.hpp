#pragma once
#include "entityt.hpp"
#include "entity_visitor.hpp"

namespace dune3d {

template <typename T> void EntityT<T>::accept(EntityVisitor &visitor) const
{
    visitor.visit(static_cast<const T &>(*this));
}

template <typename T> std::unique_ptr<Entity> EntityT<T>::clone() const
{
    return std::make_unique<T>(static_cast<const T &>(*this));
}

} // namespace dune3d
