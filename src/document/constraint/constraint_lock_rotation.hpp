#pragma once
#include "constraintt.hpp"

namespace dune3d {

class Entity;

class ConstraintLockRotation : public ConstraintT<ConstraintLockRotation> {
public:
    explicit ConstraintLockRotation(const UUID &uu);
    explicit ConstraintLockRotation(const UUID &uu, const json &j);
    json serialize() const override;

    static constexpr Type s_type = Type::LOCK_ROTATION;

    UUID m_entity;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
};
} // namespace dune3d
