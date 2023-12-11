#include "tool_common.hpp"

namespace dune3d {

class IEntityNormal;

class ToolRotate : public ToolCommon {
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
    IEntityNormal *m_entity = nullptr;
};
} // namespace dune3d
