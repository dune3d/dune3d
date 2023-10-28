#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintHV : public Constraint {
public:
    explicit ConstraintHV(const UUID &uu);
    explicit ConstraintHV(const UUID &uu, const json &j);
    json serialize() const override;

    EntityAndPoint m_entity1;
    EntityAndPoint m_entity2;

    UUID m_wrkpl;

    std::set<UUID> get_referenced_entities() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

class ConstraintHorizontal : public ConstraintHV {
public:
    using ConstraintHV::ConstraintHV;
    static constexpr Type s_type = Type::HORIZONTAL;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
};

class ConstraintVertical : public ConstraintHV {
public:
    using ConstraintHV::ConstraintHV;
    static constexpr Type s_type = Type::VERTICAL;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d
