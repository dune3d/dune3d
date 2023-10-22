#pragma once
#include "util/uuid.hpp"

namespace dune3d {
class IEntityInWorkplane {
public:
    virtual const UUID &get_workplane() const = 0;
};
} // namespace dune3d
