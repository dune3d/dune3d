#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "tool_helper_constrain.hpp"
#include "document/constraint/constraint.hpp"
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
                I::LMB_DOUBLE,
                I::CANCEL,
                I::RMB,
                I::TOGGLE_CONSTRUCTION,
                I::TOGGLE_COINCIDENT_CONSTRAINT,
                I::TOGGLE_HV_CONSTRAINT,
                I::TOGGLE_TANGENT_CONSTRAINT,
                I::TOGGLE_ARC,
                I::FLIP_ARC};
    }

    bool can_begin() override;


private:
    class EntityLine2D *m_temp_line = nullptr;
    class EntityArc2D *m_temp_arc = nullptr;

    class Entity *get_temp_entity();

    const class EntityWorkplane *m_wrkpl = nullptr;
    std::list<Entity *> m_entities;
    std::set<Constraint *> m_constraints;

    void update_tip();

    glm::dvec2 get_cursor_pos_in_plane() const;

    bool m_constrain = true;
    bool m_constrain_hv = true;
    bool m_constrain_tangent = true;

    ToolResponse end_tool();

    glm::dvec2 m_last;
    bool m_flip_arc = false;
    void set_flip_arc(bool flip);
    unsigned int get_arc_tail_point() const;
    unsigned int get_arc_head_point() const;

    unsigned int get_last_point() const;
    glm::dvec2 get_last_tangent();
    std::optional<EntityAndPoint> m_last_tangent_point;

    bool m_placing_center = false;

    void update_arc_center();
    void update_constrain_tangent();
    bool is_valid_tangent_point(const EntityAndPoint &enp);

    std::optional<Constraint::Type> get_auto_constraint() const;
};
} // namespace dune3d
