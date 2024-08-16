#pragma once
#include <glm/glm.hpp>

namespace dune3d {
class IEntityBoundingBox2D {
public:
    virtual std::pair<glm::dvec2, glm::dvec2> get_bbox() const = 0;
};
} // namespace dune3d
