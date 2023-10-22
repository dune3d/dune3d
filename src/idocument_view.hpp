#pragma once
#include "util/uuid.hpp"

namespace dune3d {

class IDocumentView {
public:
    virtual bool body_is_visible(const UUID &uu) const = 0;
    virtual bool body_solid_model_is_visible(const UUID &uu) const = 0;
    virtual bool group_is_visible(const UUID &uu) const = 0;
};

} // namespace dune3d
