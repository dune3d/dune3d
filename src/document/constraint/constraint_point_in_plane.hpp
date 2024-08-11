#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintPointInPlane : public ConstraintT<ConstraintPointInPlane> {
public:
    explicit ConstraintPointInPlane(const UUID &uu);
    explicit ConstraintPointInPlane(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::POINT_IN_PLANE;
    json serialize() const override;

    EntityAndPoint m_point;
    UUID m_line1;
    UUID m_line2;

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(
            &ConstraintPointInPlane::m_point, &ConstraintPointInPlane::m_line1, &ConstraintPointInPlane::m_line2);
};

} // namespace dune3d
