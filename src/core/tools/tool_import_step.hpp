#include "tool_common.hpp"

namespace dune3d {

class ConstraintPointDistanceBase;

class ToolImportSTEP : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;


private:
};
} // namespace dune3d
