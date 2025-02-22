#pragma once

namespace dune3d {
class ClusterContent;

class IEntityDeletePoint {
public:
    virtual bool delete_point(unsigned int point) = 0;
};

} // namespace dune3d
