#pragma once

namespace dune3d {

enum class InToolActionID {
    NONE,
    // common
    LMB,
    LMB_RELEASE,
    LMB_DOUBLE,
    RMB,
    TOGGLE_CONSTRUCTION,
    TOGGLE_COINCIDENT_CONSTRAINT,

    FLIP_ARC,
    ARC_MODE,

    CANCEL,
    COMMIT
};

} // namespace dune3d
