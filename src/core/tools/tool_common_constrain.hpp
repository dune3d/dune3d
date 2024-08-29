#pragma once
#include "tool_common.hpp"

namespace dune3d {
class ToolCommonConstrain : public ToolCommon {
public:
    using ToolCommon::ToolCommon;

    bool is_specific() override;

    ToolResponse update(const ToolArgs &args) override;

protected:
    ToolResponse commit();

    template <typename... Args> bool has_constraint_of_type(const std::set<EntityAndPoint> &enps, Args... types);
    template <typename... Args>
    bool has_constraint_of_type_in_workplane(const std::set<EntityAndPoint> &enps, Args... types);

    void reset_selection_after_constrain();
};
} // namespace dune3d
