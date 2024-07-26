#pragma once
#include <glm/glm.hpp>

namespace dune3d {

class Entity;

class IEntityMovable3D {
public:
    virtual void move(const Entity &last, const glm::dvec3 &delta, unsigned int point) = 0;
};
} // namespace dune3d
