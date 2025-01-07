#pragma once

namespace dune3d {
class IToolConstrain {
public:
    virtual bool can_preview_constrain() = 0;
};
} // namespace dune3d
