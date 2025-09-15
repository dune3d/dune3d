#include "tool_common_constrain.hpp"

namespace dune3d {

class ToolConstrainPointOnPoint : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
    bool can_preview_constrain() override
    {
        return true;
    }
    ToolID get_force_unset_workplane_tool() override;
    bool constraint_is_in_workplane() override;

protected:
    bool is_force_unset_workplane() override;
};

} // namespace dune3d
