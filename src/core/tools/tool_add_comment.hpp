#include "tool_common_constrain.hpp"

namespace dune3d {

class EntityWorkplane;
class ConstraintComment;
class EnterTextWindow;

class ToolAddComment : public ToolCommonConstrain {
public:
    using ToolCommonConstrain::ToolCommonConstrain;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;

    CanBegin can_begin() override;

private:
    const EntityWorkplane *m_wrkpl = nullptr;
    ConstraintComment *m_constraint = nullptr;

    bool m_have_win = false;
};

} // namespace dune3d
