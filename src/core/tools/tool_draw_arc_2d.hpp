#include "tool_common.hpp"
#include "tool_helper_constrain.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "document/entity/entity_and_point.hpp"
#include <optional>

namespace dune3d {

class ToolDrawArc2D : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::TOGGLE_COINCIDENT_CONSTRAINT, I::TOGGLE_CONSTRUCTION, I::FLIP_ARC,
        };
    }

    bool can_begin() override;


private:
    class EntityArc2D *m_temp_arc = nullptr;
    const class EntityWorkplane *m_wrkpl = nullptr;
    class Constraint *m_constraint_from = nullptr;
    class Constraint *m_constraint_to = nullptr;

    enum class State { FROM, TO, CENTER };
    EntityAndPoint get_entity_and_point(State state, bool flipped) const;
    State m_state = State::FROM;

    bool m_constrain = true;
    bool m_flipped = false;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;
};
} // namespace dune3d
