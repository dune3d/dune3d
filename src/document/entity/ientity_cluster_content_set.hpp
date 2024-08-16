#pragma once
#include "ientity_cluster_content.hpp"
#include <memory>

namespace dune3d {
class ClusterContent;

class IEntityClusterContentSet : public IEntityClusterContent {
public:
    virtual void set_cluster_content(std::shared_ptr<const ClusterContent>) = 0;
};

} // namespace dune3d
