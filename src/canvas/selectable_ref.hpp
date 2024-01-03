#pragma once
#include "util/uuid.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {
class SelectableRef {
public:
    UUID document;
    enum class Type { ENTITY, CONSTRAINT, SOLID_MODEL_EDGE };
    Type type;
    UUID item;
    unsigned int point;

    bool is_entity() const
    {
        return type == Type::ENTITY;
    }

    bool is_constraint() const
    {
        return type == Type::CONSTRAINT;
    }

    bool is_solid_model_edge() const
    {
        return type == Type::SOLID_MODEL_EDGE;
    }

    EntityAndPoint get_entity_and_point() const;

    friend auto operator<=>(const SelectableRef &, const SelectableRef &) = default;
    friend bool operator==(const SelectableRef &, const SelectableRef &) = default;
};
} // namespace dune3d
