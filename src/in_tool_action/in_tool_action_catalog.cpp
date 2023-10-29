#include "in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

const std::map<InToolActionID, InToolActionCatalogItem> in_tool_action_catalog = {
        {InToolActionID::LMB, {"place", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::RMB, {"cancel", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::ARC_MODE, {"arc mode", ToolID::DRAW_ARC_2D, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::FLIP_ARC, {"flip arc", ToolID::DRAW_ARC_2D, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_CONSTRUCTION,
         {"toggle construction", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT,
         {"toggle coincident constraint", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
};


#define LUT_ITEM(x)                                                                                                    \
    {                                                                                                                  \
        #x, InToolActionID::x                                                                                          \
    }

const LutEnumStr<InToolActionID> in_tool_action_lut = {
        LUT_ITEM(LMB),
        LUT_ITEM(LMB_RELEASE),
        LUT_ITEM(RMB),
        LUT_ITEM(ARC_MODE),
        LUT_ITEM(FLIP_ARC),
        LUT_ITEM(TOGGLE_CONSTRUCTION),
        LUT_ITEM(TOGGLE_COINCIDENT_CONSTRAINT),
};


} // namespace dune3d
