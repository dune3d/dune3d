#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintLinePointsPerpendicular : public ConstraintT<ConstraintLinePointsPerpendicular> {
public:
    explicit ConstraintLinePointsPerpendicular(const UUID &uu);
    explicit ConstraintLinePointsPerpendicular(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::LINE_POINTS_PERPENDICULAR;
    json serialize() const override;

    UUID m_line;
    EntityAndPoint m_point_line;
    EntityAndPoint m_point;

    constexpr static auto s_referenced_entities_and_points_tuple = std::make_tuple(
            &ConstraintLinePointsPerpendicular::m_line, &ConstraintLinePointsPerpendicular::m_point_line,
            &ConstraintLinePointsPerpendicular::m_point);
};

} // namespace dune3d
