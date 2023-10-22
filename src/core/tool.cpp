#include "tool.hpp"
#include "tool_id.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

ToolBase::ToolBase(ToolID tool_id, ICore &core, EditorInterface &intf, Flags flags)
    : m_tool_id(tool_id), m_core(core), m_intf(intf), m_is_transient((flags & Flags::TRANSIENT) != Flags::DEFAULT)
{
}


ToolResponse::ToolResponse()
{
}


ToolResponse::ToolResponse(ToolResponse::Result r) : result(r)
{
}

ToolArgs::ToolArgs() : action(InToolActionID::NONE)
{
}

} // namespace dune3d
