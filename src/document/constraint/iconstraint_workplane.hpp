#pragma once
#include "util/uuid.hpp"

namespace dune3d {
class Document;
class IConstraintWorkplane {
public:
    virtual const UUID &get_workplane(const Document &doc) const = 0;
};
} // namespace dune3d
