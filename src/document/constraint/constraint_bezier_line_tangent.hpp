#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintBezierLineTangent : public ConstraintT<ConstraintBezierLineTangent> {
public:
    explicit ConstraintBezierLineTangent(const UUID &uu);
    explicit ConstraintBezierLineTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_LINE_TANGENT;
    json serialize() const override;

    EntityAndPoint m_bezier;
    UUID m_line;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintBezierLineTangent::m_bezier, &ConstraintBezierLineTangent::m_line);
};

} // namespace dune3d
