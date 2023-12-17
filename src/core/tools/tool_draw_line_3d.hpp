#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "tool_helper_constrain.hpp"

namespace dune3d {


class ToolDrawLine3D : public virtual ToolCommon, public ToolHelperConstrain {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::LMB_DOUBLE, I::CANCEL, I::RMB, I::TOGGLE_CONSTRUCTION, I::TOGGLE_COINCIDENT_CONSTRAINT,
        };
    }

private:
    class EntityLine3D *m_temp_line = nullptr;
    std::vector<EntityLine3D *> m_temp_lines;
    class Constraint *m_constraint = nullptr;
    bool m_constrain = true;
    ToolResponse end_tool();

    glm::dvec3 m_last;


    void update_tip();
};
} // namespace dune3d
