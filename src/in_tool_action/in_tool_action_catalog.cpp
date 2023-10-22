#include "in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

const std::map<InToolActionID, InToolActionCatalogItem> in_tool_action_catalog = {
        {InToolActionID::LMB, {"place", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::RMB, {"cancel", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
};


#define LUT_ITEM(x)                                                                                                    \
    {                                                                                                                  \
        #x, InToolActionID::x                                                                                          \
    }

const LutEnumStr<InToolActionID> in_tool_action_lut = {
        LUT_ITEM(LMB),
        LUT_ITEM(LMB_RELEASE),
        LUT_ITEM(RMB),
};


} // namespace dune3d
