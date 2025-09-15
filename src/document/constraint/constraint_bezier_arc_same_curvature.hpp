#pragma once
#include "constraint_arc_arc_tangent.hpp"

namespace dune3d {

class Entity;

class ConstraintBezierArcSameCurvature : public ConstraintArcArcTangent {
public:
    using ConstraintArcArcTangent::ConstraintArcArcTangent;
    static constexpr Type s_type = Type::BEZIER_ARC_SAME_CURVATURE;
    Type get_type() const override
    {
        return s_type;
    }
    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;

    const EntityAndPoint &get_bezier(const Document &doc) const;
    const EntityAndPoint &get_arc(const Document &doc) const;
};

} // namespace dune3d
