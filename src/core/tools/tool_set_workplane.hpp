#include "tool_common.hpp"

namespace dune3d {

class ToolSetWorkplane : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override;
    bool can_begin() override;
};
} // namespace dune3d
