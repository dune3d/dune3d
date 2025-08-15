#include "tool_common.hpp"

namespace dune3d {

class ConstraintArcArcTangent;

class ToolConvertTangentConstraint : public ToolCommon {
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
    ConstraintArcArcTangent *get_constraint();
};

} // namespace dune3d
