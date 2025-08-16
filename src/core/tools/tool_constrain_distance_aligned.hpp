#include "tool_common_constrain_datum.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

class ToolConstrainDistanceAligned : public ToolCommonConstrainDatum {
public:
    using ToolCommonConstrainDatum::ToolCommonConstrainDatum;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    CanBegin can_begin() override;

    ToolID get_force_unset_workplane_tool() override;
    bool constraint_is_in_workplane() override;

protected:
    bool is_force_unset_workplane() override;

private:
    TwoPoints m_tp;
    bool m_done = false;
};
} // namespace dune3d
