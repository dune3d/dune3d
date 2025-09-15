#include "tool_common_constrain_datum.hpp"

namespace dune3d {

class ToolConstrainPointPlaneDistance : public ToolCommonConstrainDatum {
public:
    using ToolCommonConstrainDatum::ToolCommonConstrainDatum;

    ToolResponse begin(const ToolArgs &args) override;
    CanBegin can_begin() override;
    bool can_preview_constrain() override;
};

} // namespace dune3d
