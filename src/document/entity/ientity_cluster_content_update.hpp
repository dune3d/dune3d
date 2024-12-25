#pragma once
#include "ientity_cluster_content.hpp"
#include <memory>

namespace dune3d {
class ClusterContent;
class UUID;

class IEntityClusterContentUpdate : public IEntityClusterContent {
public:
    virtual void update_cluster_content_for_new_workplane(const UUID &wrkpl) = 0;
};

} // namespace dune3d
