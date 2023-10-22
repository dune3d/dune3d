#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {


class ToolDrawLine3D : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

private:
    class EntityLine3D *m_temp_line = nullptr;
    class ConstraintPointsCoincident *m_constraint = nullptr;

    void update_tip();
};
} // namespace dune3d
