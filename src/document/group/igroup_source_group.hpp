#pragma once
#include "util/uuid.hpp"

namespace dune3d {
class IGroupSourceGroup {
public:
    virtual UUID get_source_group() const = 0;
};
} // namespace dune3d
