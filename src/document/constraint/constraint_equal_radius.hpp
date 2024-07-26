#pragma once
#include "constraintt.hpp"

namespace dune3d {

class Entity;

class ConstraintEqualRadius : public ConstraintT<ConstraintEqualRadius> {
public:
    explicit ConstraintEqualRadius(const UUID &uu);
    explicit ConstraintEqualRadius(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::EQUAL_RADIUS;
    json serialize() const override;

    UUID m_entity1;
    UUID m_entity2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;
};

} // namespace dune3d
