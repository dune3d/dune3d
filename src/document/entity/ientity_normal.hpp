#pragma once
#include <glm/gtx/quaternion.hpp>

namespace dune3d {
class IEntityNormal {
public:
    virtual void set_normal(const glm::dquat &q) = 0;
    virtual glm::dquat get_normal() const = 0;
};
} // namespace dune3d
