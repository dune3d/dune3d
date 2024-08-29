#include "tool_common_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class ToolConstrainMidpoint : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
};
} // namespace dune3d
