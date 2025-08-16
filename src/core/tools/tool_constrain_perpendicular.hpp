#include "tool_common_constrain_datum.hpp"

namespace dune3d {

class ToolConstrainPerpendicular : public ToolCommonConstrainDatum {
public:
    using ToolCommonConstrainDatum::ToolCommonConstrainDatum;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
    bool can_preview_constrain() override;

    ToolID get_force_unset_workplane_tool() override;
    bool constraint_is_in_workplane() override;

protected:
    bool is_force_unset_workplane() override;
};
} // namespace dune3d
