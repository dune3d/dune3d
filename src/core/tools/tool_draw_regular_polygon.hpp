#pragma once
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include <optional>

namespace dune3d {


class ToolDrawRegularPolygon : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,         I::CANCEL,      I::RMB,           I::TOGGLE_COINCIDENT_CONSTRAINT,
                I::N_SIDES_INC, I::N_SIDES_DEC, I::ENTER_N_SIDES,
        };
    }

    bool can_begin() override;


private:
    class EntityCircle2D *m_temp_circle = nullptr;
    std::vector<class EntityLine2D *> m_sides;
    void set_n_sides(unsigned int n);
    void update_sides(const glm::dvec2 &p);
    unsigned int m_last_sides = 6;
    const class EntityWorkplane *m_wrkpl = nullptr;
    class EnterDatumWindow *m_win = nullptr;


    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
    bool m_constrain = true;
};
} // namespace dune3d
