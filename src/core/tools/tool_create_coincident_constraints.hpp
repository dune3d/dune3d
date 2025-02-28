#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class ToolCreateCoincidentConstraints : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::CANCEL, I::RMB, I::TOLERANCE_INC, I::TOLERANCE_DEC, I::ENTER_TOLERANCE,
        };
    }

    CanBegin can_begin() override;

private:
    std::set<Entity *> get_entities();
    UUID m_wrkpl;

    void apply();
    void reset();
    void update_tip();
    ToolResponse commit();

    std::set<Entity *> m_entities;
    std::set<UUID> m_entities_delete;
    std::vector<UUID> m_constraints;
    double m_tolerance = 1e-3;
    double m_last_tolerance = 1e-3;
    class EnterDatumWindow *m_win = nullptr;
};

} // namespace dune3d
