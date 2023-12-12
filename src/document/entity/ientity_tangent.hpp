#pragma once
#include <glm/glm.hpp>

namespace dune3d {
class IEntityTangent {
public:
    virtual glm::dvec2 get_tangent_at_point(unsigned int point) const = 0;
};
} // namespace dune3d
