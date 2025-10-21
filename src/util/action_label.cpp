#include "action_label.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {
ActionLabelInfo::ActionLabelInfo(InToolActionID a1)
    : action1(a1), action2(InToolActionID::NONE), action3(InToolActionID::NONE)
{
}
ActionLabelInfo::ActionLabelInfo(InToolActionID a1, const std::string &s)
    : action1(a1), action2(InToolActionID::NONE), action3(InToolActionID::NONE), label(s)
{
}
ActionLabelInfo::ActionLabelInfo(InToolActionID a1, InToolActionID a2, const std::string &s)
    : action1(a1), action2(a2), action3(InToolActionID::NONE), label(s)
{
}
ActionLabelInfo::ActionLabelInfo(InToolActionID a1, InToolActionID a2, InToolActionID a3, const std::string &s)
    : action1(a1), action2(a2), action3(a3), label(s)
{
}
} // namespace dune3d
