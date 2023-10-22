#pragma once
#include "in_tool_action.hpp"
#include "util/lut.hpp"
#include <map>

namespace dune3d {

enum class ToolID;

class InToolActionCatalogItem {
public:
    enum Flags {
        FLAGS_DEFAULT = 0,
        FLAGS_NO_PREFERENCES = (1 << 5),
    };

    InToolActionCatalogItem(const std::string &n, ToolID t, int fl = FLAGS_DEFAULT)
        : name(n), tool(t), flags(static_cast<Flags>(fl)){};

    const std::string name;
    ToolID tool;
    const Flags flags;
};

extern const std::map<InToolActionID, InToolActionCatalogItem> in_tool_action_catalog;
extern const LutEnumStr<InToolActionID> in_tool_action_lut;
} // namespace dune3d
