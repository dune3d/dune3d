#pragma once
#include "constraint.hpp"

namespace dune3d {

class Entity;

class ConstraintLockRotation : public Constraint {
public:
    explicit ConstraintLockRotation(const UUID &uu);
    explicit ConstraintLockRotation(const UUID &uu, const json &j);
    json serialize() const override;

    static constexpr Type s_type = Type::LOCK_ROTATION;
    Type get_type() const override
    {
        return s_type;
    }

    std::unique_ptr<Constraint> clone() const override;

    UUID m_entity;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;
};
} // namespace dune3d
