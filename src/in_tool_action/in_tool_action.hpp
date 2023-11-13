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
    TOGGLE_HV_CONSTRAINT,
    TOGGLE_TANGENT_CONSTRAINT,

    FLIP_ARC,
    ARC_MODE,

    TOGGLE_ARC,

    CANCEL,
    COMMIT
};

} // namespace dune3d
