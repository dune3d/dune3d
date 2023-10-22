#pragma once
#include "util/uuid.hpp"

namespace dune3d {
class SelectableRef {
public:
    UUID document;
    enum class Type { ENTITY, CONSTRAINT, SOLID_MODEL_EDGE };
    Type type;
    UUID item;
    unsigned int point;
    friend auto operator<=>(const SelectableRef &, const SelectableRef &) = default;
    friend bool operator==(const SelectableRef &, const SelectableRef &) = default;
};
} // namespace dune3d
