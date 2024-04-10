#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

class ToolConstrainSymmetricLine : public ToolCommon {
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
    TwoPoints m_points;
};
} // namespace dune3d
