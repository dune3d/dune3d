#pragma once
#include "ientity_in_workplane.hpp"

namespace dune3d {
class IEntityInWorkplaneSet : public IEntityInWorkplane {
public:
    virtual void set_workplane(const UUID &uu) = 0;
};
} // namespace dune3d
