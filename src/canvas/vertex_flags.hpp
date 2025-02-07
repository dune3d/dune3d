#pragma once
#include <stdint.h>
#include "bitmask_operators.hpp"


namespace dune3d {
enum class CanvasVertexFlags : uint32_t {
    DEFAULT = 0,
    SELECTED = (1 << 0),
    HOVER = (1 << 1),
    INACTIVE = (1 << 2),
    CONSTRAINT = (1 << 3),
    CONSTRUCTION = (1 << 4),
    HIGHLIGHT = (1 << 5),
    SCREEN = (1 << 6),
    LINE_THIN = (1 << 7),
    COLOR_MASK = SELECTED | HOVER | INACTIVE | CONSTRAINT | CONSTRUCTION | HIGHLIGHT,
};
}

template <> struct enable_bitmask_operators<dune3d::CanvasVertexFlags> {
    static constexpr bool enable = true;
};
