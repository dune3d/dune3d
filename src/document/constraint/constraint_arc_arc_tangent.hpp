#pragma once
#include "constraint.hpp"
#include "document/entity/entity_and_point.hpp"

namespace dune3d {

class Entity;

class ConstraintArcArcTangent : public Constraint {
public:
    explicit ConstraintArcArcTangent(const UUID &uu);
    explicit ConstraintArcArcTangent(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::ARC_ARC_TANGENT;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Constraint> clone() const override;

    EntityAndPoint m_arc1;
    EntityAndPoint m_arc2;

    std::set<EntityAndPoint> get_referenced_entities_and_points() const override;

    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d
