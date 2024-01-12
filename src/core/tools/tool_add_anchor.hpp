#include "tool_common.hpp"

namespace dune3d {

class EntitySTEP;

class ToolAddAnchor : public ToolCommon {
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
    EntitySTEP *m_step = nullptr;
    std::set<unsigned int> m_anchors;
};
} // namespace dune3d
