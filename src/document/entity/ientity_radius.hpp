#pragma once
#include <glm/glm.hpp>

namespace dune3d {
class IEntityRadius {
public:
    virtual double get_radius() const = 0;
    virtual glm::dvec2 get_center() const = 0;
};
} // namespace dune3d
