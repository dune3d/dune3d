#pragma once
#include "action.hpp"
#include "util/lut.hpp"
#include <map>

namespace dune3d {
class ActionCatalogItem {
public:
    enum Flags {
        FLAGS_DEFAULT = 0,
        FLAGS_IN_TOOL = (1 << 1),
        FLAGS_NO_POPOVER = (1 << 2),
        FLAGS_NO_MENU = (1 << 3),
        FLAGS_SPECIFIC = (1 << 4),
        FLAGS_NO_PREFERENCES = (1 << 5),
    };

    ActionCatalogItem(const std::string &n, ActionGroup gr, int fl = FLAGS_DEFAULT)
        : name(n), group(gr), flags(static_cast<Flags>(fl)){};

    const std::string name;
    const ActionGroup group;
    const Flags flags;
};

extern const std::map<ActionToolID, ActionCatalogItem> action_catalog;
extern const LutEnumStr<ActionID> action_lut;
extern const LutEnumStr<ToolID> tool_lut;

extern const std::vector<std::pair<ActionGroup, std::string>> action_group_catalog;
} // namespace dune3d
