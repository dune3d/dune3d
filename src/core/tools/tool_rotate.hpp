#include "tool_common.hpp"

namespace dune3d {

class IEntityNormal;
class Entity;

class ToolRotate : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }

    CanBegin can_begin() override;

private:
    IEntityNormal *m_entity_normal = nullptr;
    Entity *m_entity = nullptr;
};
} // namespace dune3d
