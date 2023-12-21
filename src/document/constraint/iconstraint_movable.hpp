#pragma once
#include "util/uuid.hpp"
#include <glm/glm.hpp>

namespace dune3d {
class Document;
class IConstraintMovable {
public:
    virtual glm::dvec3 get_origin(const Document &doc) const = 0;
    virtual glm::dvec3 get_offset() const = 0;
    virtual void set_offset(const glm::dvec3 &offset) = 0;
};
} // namespace dune3d
