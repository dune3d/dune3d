#pragma once
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <optional>

namespace dune3d {


class ToolDrawPoint2D : public virtual ToolCommon, public ToolHelperConstrain {
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

    CanBegin can_begin() override;


private:
    class EntityPoint2D *m_temp_point = nullptr;
    const class EntityWorkplane *m_wrkpl = nullptr;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
    bool m_constrain = true;
};
} // namespace dune3d
