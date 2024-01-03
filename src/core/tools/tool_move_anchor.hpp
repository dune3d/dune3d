#include "tool_common.hpp"

namespace dune3d {

class EntitySTEP;

class ToolMoveAnchor : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    bool can_begin() override;


private:
    EntitySTEP *m_step = nullptr;
    unsigned int m_anchor;
};
} // namespace dune3d
