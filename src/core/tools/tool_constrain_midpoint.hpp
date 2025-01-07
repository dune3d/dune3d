#include "tool_common_constrain.hpp"

namespace dune3d {

class ToolConstrainMidpoint : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
    bool can_preview_constrain() override
    {
        return true;
    }
};
} // namespace dune3d
