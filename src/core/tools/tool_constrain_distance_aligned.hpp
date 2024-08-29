#include "tool_common_constrain.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

class ToolConstrainDistanceAligned : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    CanBegin can_begin() override;

private:
    TwoPoints m_tp;
};
} // namespace dune3d
