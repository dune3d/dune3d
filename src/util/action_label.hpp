#pragma once
#include <string>

namespace dune3d {
enum class InToolActionID;

class ActionLabelInfo {
public:
    ActionLabelInfo(InToolActionID a1);
    ActionLabelInfo(InToolActionID a1, const std::string &s);
    ActionLabelInfo(InToolActionID a1, InToolActionID a2, const std::string &s);
    ActionLabelInfo(InToolActionID a1, InToolActionID a2, InToolActionID a3, const std::string &s);

    InToolActionID action1;
    InToolActionID action2;
    InToolActionID action3;
    std::string label;

    bool operator==(const ActionLabelInfo &other) const = default;
};
} // namespace dune3d
