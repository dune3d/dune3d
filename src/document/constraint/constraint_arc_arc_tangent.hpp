#pragma once
#include "constraintt.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintArcArcTangent : public ConstraintT<ConstraintArcArcTangent> {
public:
    explicit ConstraintArcArcTangent(const UUID &uu);
    explicit ConstraintArcArcTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::ARC_ARC_TANGENT;
    json serialize() const override;

    EntityAndPoint m_arc1;
    EntityAndPoint m_arc2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    bool replace_point(const EntityAndPoint &old_point, const EntityAndPoint &new_point) override;
};

} // namespace dune3d
