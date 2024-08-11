#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintArcLineTangent : public ConstraintT<ConstraintArcLineTangent> {
public:
    explicit ConstraintArcLineTangent(const UUID &uu);
    explicit ConstraintArcLineTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::ARC_LINE_TANGENT;
    json serialize() const override;

    EntityAndPoint m_arc;
    UUID m_line;

    constexpr static auto s_referenced_entities_and_points_tuple =
            std::make_tuple(&ConstraintArcLineTangent::m_arc, &ConstraintArcLineTangent::m_line);
};

} // namespace dune3d
