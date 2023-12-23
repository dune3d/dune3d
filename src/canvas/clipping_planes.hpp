#pragma once

namespace dune3d {
struct ClippingPlanes {
    struct Plane {
        bool enabled = false;
        enum class Operation { CLIP_LESS, CLIP_GREATER };
        Operation operation = Operation::CLIP_LESS;
        double value = 0;
    };

    Plane x;
    Plane y;
    Plane z;
};

} // namespace dune3d
