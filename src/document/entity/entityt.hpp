#pragma once
#include "entity.hpp"

namespace dune3d {

class EntityVisitor;

template <typename T> class EntityT : public Entity {
public:
    using Entity::Entity;
    using Base = EntityT<T>;

    Type get_type() const override
    {
        return T::s_type;
    }

    void accept(EntityVisitor &visitor) const override;
    std::unique_ptr<Entity> clone() const override;
};

} // namespace dune3d
