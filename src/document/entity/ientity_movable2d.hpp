#pragma once
#include <glm/glm.hpp>

namespace dune3d {

class Entity;

class IEntityMovable2D {
public:
    virtual void move(const Entity &last, const glm::dvec2 &delta, unsigned int point) = 0;
};
} // namespace dune3d
