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

    CLEAR_EDGES,

    N_SIDES_INC,
    N_SIDES_DEC,
    ENTER_N_SIDES,

    CANCEL,
    COMMIT
};

} // namespace dune3d
