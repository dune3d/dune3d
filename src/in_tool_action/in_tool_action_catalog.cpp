#include "in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

const std::map<InToolActionID, InToolActionCatalogItem> in_tool_action_catalog = {
        {InToolActionID::LMB, {"place", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::RMB, {"cancel", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::ARC_MODE, {"arc mode", ToolID::DRAW_ARC_2D, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_ARC, {"toggle arc", ToolID::DRAW_CONTOUR, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_BEZIER,
         {"toggle bezier", ToolID::DRAW_CONTOUR, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::FLIP_ARC, {"flip arc", ToolID::DRAW_ARC_2D, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_CONSTRUCTION,
         {"toggle construction", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_COINCIDENT_CONSTRAINT,
         {"toggle coincident constraint", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_HV_CONSTRAINT,
         {"toggle horizontal/vertical constraint", ToolID::DRAW_CONTOUR, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_TANGENT_CONSTRAINT,
         {"toggle tangent constraint", ToolID::DRAW_CONTOUR, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::CLEAR_EDGES, {"clear edges", ToolID::SELECT_EDGES, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::CLEAR_SPINE_ENTITIES,
         {"clear spine entities", ToolID::SELECT_SPINE_ENTITIES, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::N_SIDES_INC,
         {"increase sides", ToolID::DRAW_REGULAR_POLYGON, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::N_SIDES_DEC,
         {"decrease sides", ToolID::DRAW_REGULAR_POLYGON, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ENTER_N_SIDES,
         {"enter sides", ToolID::DRAW_REGULAR_POLYGON, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_RECTANGLE_MODE,
         {"toggle rectangle mode", ToolID::DRAW_RECTANGLE, InToolActionCatalogItem::FLAGS_DEFAULT}},
};


#define LUT_ITEM(x) {#x, InToolActionID::x}

const LutEnumStr<InToolActionID> in_tool_action_lut = {
        LUT_ITEM(LMB),
        LUT_ITEM(RMB),
        LUT_ITEM(ARC_MODE),
        LUT_ITEM(FLIP_ARC),
        LUT_ITEM(TOGGLE_ARC),
        LUT_ITEM(TOGGLE_BEZIER),
        LUT_ITEM(TOGGLE_CONSTRUCTION),
        LUT_ITEM(TOGGLE_COINCIDENT_CONSTRAINT),
        LUT_ITEM(TOGGLE_HV_CONSTRAINT),
        LUT_ITEM(TOGGLE_TANGENT_CONSTRAINT),
        LUT_ITEM(CLEAR_EDGES),
        LUT_ITEM(N_SIDES_INC),
        LUT_ITEM(N_SIDES_DEC),
        LUT_ITEM(ENTER_N_SIDES),
        LUT_ITEM(TOGGLE_RECTANGLE_MODE),
        LUT_ITEM(CLEAR_SPINE_ENTITIES),
};


} // namespace dune3d
