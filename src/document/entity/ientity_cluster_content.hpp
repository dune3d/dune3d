#pragma once

namespace dune3d {
class ClusterContent;

class IEntityClusterContent {
public:
    virtual const ClusterContent &get_cluster_content() const = 0;
};

} // namespace dune3d
