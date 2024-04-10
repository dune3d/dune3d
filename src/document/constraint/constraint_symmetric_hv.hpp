#pragma once
#include "constraint.hpp"
#include "iconstraint_workplane.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintSymmetricHV : public Constraint, public IConstraintWorkplane {
public:
    explicit ConstraintSymmetricHV(const UUID &uu);
    explicit ConstraintSymmetricHV(const UUID &uu, const json &j);
    json serialize() const override;

    EntityAndPoint m_entity1;
    EntityAndPoint m_entity2;

    UUID m_wrkpl;

    const UUID &get_workplane(const Document &doc) const override
    {
        return m_wrkpl;
    }

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

class ConstraintSymmetricHorizontal : public ConstraintSymmetricHV {
public:
    using ConstraintSymmetricHV::ConstraintSymmetricHV;
    static constexpr Type s_type = Type::SYMMETRIC_HORIZONTAL;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
};

class ConstraintSymmetricVertical : public ConstraintSymmetricHV {
public:
    using ConstraintSymmetricHV::ConstraintSymmetricHV;
    static constexpr Type s_type = Type::SYMMETRIC_VERTICAL;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;
};

} // namespace dune3d
