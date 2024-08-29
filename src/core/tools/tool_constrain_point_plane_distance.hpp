#include "tool_common_constrain.hpp"

namespace dune3d {

class ToolConstrainPointPlaneDistance : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
};

} // namespace dune3d
