#pragma once
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <optional>
#include <array>

namespace dune3d {

class ToolDrawRectangle : public virtual ToolCommon, public ToolHelperConstrain {
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
                I::TOGGLE_CONSTRUCTION,
                I::TOGGLE_COINCIDENT_CONSTRAINT,
                I::TOGGLE_RECTANGLE_MODE,
        };
    }

    bool can_begin() override;


private:
    std::array<class EntityLine2D *, 4> m_lines;
    const class EntityWorkplane *m_wrkpl = nullptr;

    glm::dvec2 m_first_point;
    enum class Mode { CENTER, CORNER };
    Mode m_mode = Mode::CORNER;
    std::optional<Constraint::Type> m_first_constraint;
    EntityAndPoint m_first_enp;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
    bool m_constrain = true;
};
} // namespace dune3d
