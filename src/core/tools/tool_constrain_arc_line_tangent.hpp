#include "tool_common.hpp"

namespace dune3d {

enum class EntityType;

class ToolConstrainArcLineTangent : public ToolCommon {
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
    EntityType get_curve_type() const;
};

} // namespace dune3d
