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

    SelectableRef() = default;
    SelectableRef(const UUID& doc, Type t, const UUID& itm, unsigned int pt)
        : document(doc), type(t), item(itm), point(pt) {};
    SelectableRef(const UUID& doc, Type t, const UUID& itm)
        : document(doc), type(t), item(itm), point(0) {};

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
