#include "tool_common.hpp"

namespace dune3d {

class ConstraintPointDistanceBase;
class ConstraintDiameterRadius;

class ToolEnterDatum : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool is_specific() override
    {
        return true;
    }


private:
    ConstraintPointDistanceBase *m_constraint_point_distance = nullptr;
    ConstraintDiameterRadius *m_constraint_diameter_radius = nullptr;
};
} // namespace dune3d
