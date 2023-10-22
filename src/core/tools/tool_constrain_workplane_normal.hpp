#include "tool_common.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

class EntityWorkplane;
class Entity;

class ToolConstrainWorkplaneNormal : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }
    bool can_begin() override;


private:
    EntityWorkplane *m_wrkpl = nullptr;
    Entity *m_line1 = nullptr;
};
} // namespace dune3d
