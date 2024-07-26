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

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d
