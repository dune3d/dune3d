#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "tool_helper_constrain.hpp"
#include <optional>
#include <list>

namespace dune3d {

class ToolDrawContour : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {I::LMB,
                I::CANCEL,
                I::RMB,
                I::TOGGLE_CONSTRUCTION,
                I::TOGGLE_COINCIDENT_CONSTRAINT,
                I::TOGGLE_HV_CONSTRAINT,
                I::TOGGLE_TANGENT_CONSTRAINT,
                I::TOGGLE_ARC,
                I::TOGGLE_BEZIER,
                I::FLIP_ARC};
    }

    CanBegin can_begin() override;


private:
    bool is_draw_contour() const;

    class EntityLine2D *m_temp_line = nullptr;
    class EntityArc2D *m_temp_arc = nullptr;
    class EntityBezier2D *m_temp_bezier = nullptr;

    class Entity *get_temp_entity();

    const class EntityWorkplane *m_wrkpl = nullptr;
    std::vector<Entity *> m_entities;
    std::set<Constraint *> m_constraints;
    bool m_tangent_valid = true;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;

    bool m_constrain = true;
    bool m_constrain_hv = true;
    bool m_constrain_tangent = true;
    bool m_constrain_tangent_head = true;
    bool m_has_tangent_head = false;
    std::optional<ConstraintType> get_head_constraint(const Entity &en_head, const Entity &en_target);
    Constraint *constrain_point_and_add_head_tangent_constraint(const UUID &wrkpl,
                                                                const EntityAndPoint &enp_to_constrain);
    bool check_close_path();
    bool m_close_path = false;

    ToolResponse end_tool();

    glm::dvec2 m_last;
    bool m_flip_arc = false;
    void set_flip_arc(bool flip);
    unsigned int get_arc_tail_point() const;
    unsigned int get_arc_head_point() const;
    unsigned int get_head_point() const;

    unsigned int get_last_point() const;
    glm::dvec2 get_last_tangent();
    glm::dvec2 get_tangent_for_point(const EntityAndPoint &enp);
    std::optional<EntityAndPoint> m_last_tangent_point;
    std::optional<EntityAndPoint> m_bezier_head_tangent_point;

    enum class State { NORMAL, CENTER, BEZIER_C1, BEZIER_C2 };
    State m_state = State::NORMAL;
    bool is_placing_center() const
    {
        return m_state == State::CENTER;
    }

    void update_arc_center();
    void update_bezier_controls();
    void update_constrain_tangent();
    bool is_valid_tangent_point(const EntityAndPoint &enp);

    std::optional<ConstraintType> get_auto_constraint() const;
};
} // namespace dune3d
