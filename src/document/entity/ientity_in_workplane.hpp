#pragma once
#include "util/uuid.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class IEntityInWorkplane {
public:
    virtual const UUID &get_workplane() const = 0;
    virtual glm::dvec2 get_point_in_workplane(unsigned int point) const = 0;
};
} // namespace dune3d
