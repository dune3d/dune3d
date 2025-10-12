#pragma once

namespace dune3d {

enum class ToolID;

class IToolConstrain {
public:
    virtual bool can_preview_constrain() = 0;
    virtual ToolID get_force_unset_workplane_tool() = 0;
    virtual bool constraint_is_in_workplane() = 0;
};
} // namespace dune3d
