#pragma once
#include "util/uuid.hpp"
#include <set>

namespace dune3d {

class Document;

class IGroupSourceGroup {
public:
    virtual std::set<UUID> get_source_groups(const Document &doc) const = 0;
};
} // namespace dune3d
