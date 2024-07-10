#pragma once
#include "util/uuid.hpp"
#include <set>

namespace dune3d {
class IGroupSourceGroup {
public:
    virtual std::set<UUID> get_source_groups() const = 0;
};
} // namespace dune3d
