#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <optional>

namespace dune3d {


class ToolDrawCircle2D : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::TOGGLE_CONSTRUCTION, I::TOGGLE_COINCIDENT_CONSTRAINT,
        };
    }

    bool can_begin() override;


private:
    class EntityCircle2D *m_temp_circle = nullptr;
    const class EntityWorkplane *m_wrkpl = nullptr;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
    bool m_constrain = true;
};
} // namespace dune3d
