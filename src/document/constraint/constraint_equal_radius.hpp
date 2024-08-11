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

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintEqualRadius::m_entity1, &ConstraintEqualRadius::m_entity2);
};

} // namespace dune3d
