#include "tool_common.hpp"

namespace dune3d {

class EntityCluster;

class ToolAddClusterAnchor : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    CanBegin can_begin() override;


private:
    EntityCluster *m_cluster = nullptr;
    std::set<EntityAndPoint> m_anchors;
};
} // namespace dune3d
