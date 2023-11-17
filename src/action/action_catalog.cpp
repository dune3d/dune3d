#include "action_catalog.hpp"
#include "core/tool_id.hpp"
#include "action_id.hpp"

namespace dune3d {
const std::map<ActionToolID, ActionCatalogItem> action_catalog = {
        {ActionID::SAVE_ALL, {"Save all", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::SAVE, {"Save", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::SAVE_AS, {"Save as", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::EXPORT_SOLID_MODEL_STL, {"Export STL", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::EXPORT_SOLID_MODEL_STEP, {"Export STEP", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DRAW_CONTOUR, {"Draw contour", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DRAW_CONTOUR_FROM_POINT,
         {"Draw contour", ActionGroup::DRAW,
          ActionCatalogItem::FLAGS_NO_MENU | ActionCatalogItem::FLAGS_NO_POPOVER
                  | ActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {ToolID::DRAW_LINE_3D, {"Draw line", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DRAW_ARC_2D, {"Draw arc in workplane", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DRAW_CIRCLE_2D, {"Draw circle in workplane", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DRAW_WORKPLANE, {"Draw workplane", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::MOVE, {"Move", ActionGroup::MOVE, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::DELETE, {"Delete", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::POPOVER, {"Popover", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_NO_POPOVER}},
        {ToolID::SET_WORKPLANE, {"Set workplane", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::UNSET_WORKPLANE, {"Unset workplane", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_COINCIDENT,
         {"Constrain coincident", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_HORIZONTAL,
         {"Constrain horizontal", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_VERTICAL, {"Constrain vertical", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_DISTANCE, {"Constrain distance", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_DISTANCE_HORIZONTAL,
         {"Constrain horizontal distance", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_DISTANCE_VERTICAL,
         {"Constrain vertical distance", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_SAME_ORIENTATION,
         {"Constrain same orientation", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_PARALLEL, {"Constrain parallel", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_WORKPLANE_NORMAL,
         {"Constrain workplane normal", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_MIDPOINT, {"Constrain midpoint", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_EQUAL_LENGTH,
         {"Constrain equal length", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_EQUAL_RADIUS,
         {"Constrain equal radius", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_DIAMETER, {"Constrain diameter", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_RADIUS, {"Constrain radius", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::CONSTRAIN_PERPENDICULAR,
         {"Constrain perpendicular", ActionGroup::CONSTRAIN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::TOGGLE_CONSTRUCTION, {"Toggle construction", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::SET_CONSTRUCTION, {"Set construction", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::UNSET_CONSTRUCTION, {"Unset construction", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::ENTER_DATUM, {"Enter datum", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::ADD_ANCHOR, {"Add anchor", ActionGroup::DRAW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::ENTER_DATUM, {"Enter datum", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::IMPORT_STEP, {"Import STEP", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::UNDO, {"Undo", ActionGroup::UNDO, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::REDO, {"Redo", ActionGroup::UNDO, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::CLOSE_DOCUMENT, {"Close document", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::OPEN_DOCUMENT, {"Open document", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::NEW_DOCUMENT, {"New document", ActionGroup::DOCUMENT, ActionCatalogItem::FLAGS_DEFAULT}},
        {ToolID::SELECT_EDGES, {"Select edges", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::PREVIOUS_GROUP, {"Previous group", ActionGroup::VIEW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::NEXT_GROUP, {"Next group", ActionGroup::VIEW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::PREFERENCES, {"Preferences", ActionGroup::UNKNOWN, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::TOGGLE_SOLID_MODEL, {"Toggle solid model", ActionGroup::VIEW, ActionCatalogItem::FLAGS_DEFAULT}},
        {ActionID::VIEW_ALL, {"Reset view", ActionGroup::VIEW, ActionCatalogItem::FLAGS_IN_TOOL}},
        {ActionID::VIEW_PERSP, {"View perspective", ActionGroup::VIEW, ActionCatalogItem::FLAGS_IN_TOOL}},
        {ActionID::VIEW_ORTHO, {"View orthographic", ActionGroup::VIEW, ActionCatalogItem::FLAGS_IN_TOOL}},
        {ActionID::VIEW_TOGGLE_PERSP_ORTHO,
         {"Toggle persp/ortho", ActionGroup::VIEW, ActionCatalogItem::FLAGS_IN_TOOL}},
};


const std::vector<std::pair<ActionGroup, std::string>> action_group_catalog = {
        {ActionGroup::CONSTRAIN, "Constrain"}, {ActionGroup::DRAW, "Draw"},         {ActionGroup::MOVE, "Move"},
        {ActionGroup::UNDO, "Undo"},           {ActionGroup::DOCUMENT, "Document"}, {ActionGroup::VIEW, "View"},
        {ActionGroup::UNKNOWN, "Misc"},
};

#define ACTION_LUT_ITEM(x)                                                                                             \
    {                                                                                                                  \
        #x, ActionID::x                                                                                                \
    }

const LutEnumStr<ActionID> action_lut = {
        ACTION_LUT_ITEM(SAVE_ALL),
        ACTION_LUT_ITEM(SAVE),
        ACTION_LUT_ITEM(SAVE_AS),
        ACTION_LUT_ITEM(EXPORT_SOLID_MODEL_STL),
        ACTION_LUT_ITEM(EXPORT_SOLID_MODEL_STEP),
        ACTION_LUT_ITEM(UNDO),
        ACTION_LUT_ITEM(REDO),
        ACTION_LUT_ITEM(POPOVER),
        ACTION_LUT_ITEM(CLOSE_DOCUMENT),
        ACTION_LUT_ITEM(OPEN_DOCUMENT),
        ACTION_LUT_ITEM(NEW_DOCUMENT),
        ACTION_LUT_ITEM(PREVIOUS_GROUP),
        ACTION_LUT_ITEM(NEXT_GROUP),
        ACTION_LUT_ITEM(PREFERENCES),
        ACTION_LUT_ITEM(TOGGLE_SOLID_MODEL),
        ACTION_LUT_ITEM(VIEW_ALL),
        ACTION_LUT_ITEM(VIEW_PERSP),
        ACTION_LUT_ITEM(VIEW_ORTHO),
        ACTION_LUT_ITEM(VIEW_TOGGLE_PERSP_ORTHO),
};

#define TOOL_LUT_ITEM(x)                                                                                               \
    {                                                                                                                  \
        #x, ToolID::x                                                                                                  \
    }

const LutEnumStr<ToolID> tool_lut = {
        TOOL_LUT_ITEM(NONE),
        TOOL_LUT_ITEM(MOVE),
        TOOL_LUT_ITEM(DELETE),
        TOOL_LUT_ITEM(DRAW_ARC_2D),
        TOOL_LUT_ITEM(DRAW_CIRCLE_2D),
        TOOL_LUT_ITEM(DRAW_LINE_3D),
        TOOL_LUT_ITEM(DRAW_WORKPLANE),
        TOOL_LUT_ITEM(DRAW_CONTOUR),
        TOOL_LUT_ITEM(DRAW_CONTOUR_FROM_POINT),
        TOOL_LUT_ITEM(CONSTRAIN_COINCIDENT),
        TOOL_LUT_ITEM(CONSTRAIN_HORIZONTAL),
        TOOL_LUT_ITEM(CONSTRAIN_VERTICAL),
        TOOL_LUT_ITEM(CONSTRAIN_DISTANCE),
        TOOL_LUT_ITEM(CONSTRAIN_DISTANCE_VERTICAL),
        TOOL_LUT_ITEM(CONSTRAIN_DISTANCE_HORIZONTAL),
        TOOL_LUT_ITEM(CONSTRAIN_SAME_ORIENTATION),
        TOOL_LUT_ITEM(CONSTRAIN_PARALLEL),
        TOOL_LUT_ITEM(CONSTRAIN_WORKPLANE_NORMAL),
        TOOL_LUT_ITEM(CONSTRAIN_MIDPOINT),
        TOOL_LUT_ITEM(CONSTRAIN_EQUAL_LENGTH),
        TOOL_LUT_ITEM(CONSTRAIN_EQUAL_RADIUS),
        TOOL_LUT_ITEM(CONSTRAIN_RADIUS),
        TOOL_LUT_ITEM(CONSTRAIN_DIAMETER),
        TOOL_LUT_ITEM(CONSTRAIN_PERPENDICULAR),
        TOOL_LUT_ITEM(ENTER_DATUM),
        TOOL_LUT_ITEM(ADD_ANCHOR),
        TOOL_LUT_ITEM(TOGGLE_CONSTRUCTION),
        TOOL_LUT_ITEM(SET_CONSTRUCTION),
        TOOL_LUT_ITEM(UNSET_CONSTRUCTION),
        TOOL_LUT_ITEM(IMPORT_STEP),
        TOOL_LUT_ITEM(SELECT_EDGES),
        TOOL_LUT_ITEM(SET_WORKPLANE),
        TOOL_LUT_ITEM(UNSET_WORKPLANE),
};


} // namespace dune3d
