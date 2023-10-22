#include "core/tool.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class EntitySTEP;

class ToolAddAnchor : public ToolBase {
public:
    using ToolBase::ToolBase;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    bool can_begin() override;


private:
    EntitySTEP *m_step = nullptr;
    std::set<unsigned int> m_anchors;
};
} // namespace dune3d
