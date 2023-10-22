#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <optional>

namespace dune3d {


class ToolDrawArc2D : public ToolCommon {
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

    bool can_begin() override;


private:
    class EntityArc2D *m_temp_arc = nullptr;
    const class EntityWorkplane *m_wrkpl = nullptr;
    class ConstraintPointsCoincident *m_constraint = nullptr;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
};
} // namespace dune3d
