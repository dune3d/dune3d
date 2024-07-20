#include "tool_common.hpp"

namespace dune3d {

class ToolImportDXF : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;

    CanBegin can_begin() override;


private:
};
} // namespace dune3d
