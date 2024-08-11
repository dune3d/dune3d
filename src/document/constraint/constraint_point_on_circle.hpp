#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointOnCircle : public ConstraintT<ConstraintPointOnCircle> {
public:
    explicit ConstraintPointOnCircle(const UUID &uu);
    explicit ConstraintPointOnCircle(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_ON_CIRCLE;
    json serialize() const override;

    EntityAndPoint m_point;
    UUID m_circle;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintPointOnCircle::m_point, &ConstraintPointOnCircle::m_circle);
};

} // namespace dune3d
