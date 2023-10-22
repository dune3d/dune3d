#pragma once
#include <string>

namespace dune3d {
enum class InToolActionID;

class ActionLabelInfo {
public:
    ActionLabelInfo(InToolActionID a1);
    ActionLabelInfo(InToolActionID a1, const std::string &s);
    ActionLabelInfo(InToolActionID a1, InToolActionID a2, const std::string &s);

    InToolActionID action1;
    InToolActionID action2;
    std::string label;
    std::pair<InToolActionID, InToolActionID> get_key() const
    {
        return std::make_pair(action1, action2);
    }

    bool operator==(const ActionLabelInfo &other) const
    {
        return action1 == other.action1 && action2 == other.action2 && label == other.label;
    }
};
} // namespace dune3d
