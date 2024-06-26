#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

class ToolConstrainDistanceAligned : public ToolCommon {
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
    TwoPoints m_tp;
};
} // namespace dune3d
