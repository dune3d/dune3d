#pragma once
#include "action.hpp"

namespace dune3d {
enum class ActionID {
    SAVE,
    SAVE_AS,
    SAVE_ALL,
    UNDO,
    REDO,
    POPOVER,
    SET_WORKPLANE,
    UNSET_WORKPLANE,
    CLOSE_DOCUMENT,
    OPEN_DOCUMENT,
    NEW_DOCUMENT,
    EXPORT_SOLID_MODEL_STL,
    EXPORT_SOLID_MODEL_STEP,
    PREVIOUS_GROUP,
    NEXT_GROUP,
    PREFERENCES,
    TOGGLE_SOLID_MODEL,
    VIEW_ALL,
};
}
