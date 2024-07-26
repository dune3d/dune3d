#pragma once
#include <glm/glm.hpp>

namespace dune3d {

class Entity;

class IEntityMovable2DIntialPos {
public:
    virtual void move(const Entity &last, const glm::dvec2 &intial_pos, const glm::dvec2 &pos, unsigned int point) = 0;
};
} // namespace dune3d
