#include "tool_common_constrain.hpp"

namespace dune3d {

class ConstraintWorkplaneNormal;

class ToolConstrainWorkplaneNormal : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    CanBegin can_begin() override;

private:
    UUID m_wrkpl;
    UUID get_wrkpl();
    UUID m_line1;
    ConstraintWorkplaneNormal *m_constraint = nullptr;
    void update_tip();
};
} // namespace dune3d
