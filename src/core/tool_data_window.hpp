#pragma once
#include "tool_data.hpp"

namespace dune3d {
class ToolDataWindow : public ToolData {
public:
    enum class Event { NONE, CLOSE, OK, UPDATE };
    Event event = Event::NONE;
};
} // namespace dune3d
