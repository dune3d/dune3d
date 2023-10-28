#include "tool_common.hpp"

namespace dune3d {

class IConstraintDatum;

class ToolEnterDatum : public ToolCommon {
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
    IConstraintDatum *m_constraint = nullptr;
};
} // namespace dune3d
