#pragma once
#include <glm/glm.hpp>

namespace dune3d {
class EntityWorkplane;
class IEntityTangentProjected {
public:
    virtual glm::dvec2 get_tangent_in_workplane(double t, const EntityWorkplane &wrkpl) const = 0;
};
} // namespace dune3d
