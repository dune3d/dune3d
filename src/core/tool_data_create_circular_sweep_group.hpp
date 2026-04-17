#pragma once
#include "tool_data.hpp"

namespace dune3d {

class ToolDataCreateCircularSweepGroup : public ToolData {
public:
    explicit ToolDataCreateCircularSweepGroup(bool with_body) : with_body(with_body)
    {
    }

    bool with_body = false;
};
} // namespace dune3d
