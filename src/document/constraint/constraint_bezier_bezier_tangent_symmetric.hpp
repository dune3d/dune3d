#pragma once
#include "constraint_arc_arc_tangent.hpp"

namespace dune3d {

class Entity;

class ConstraintBezierBezierTangentSymmetric : public ConstraintArcArcTangent {
public:
    using ConstraintArcArcTangent::ConstraintArcArcTangent;
    static constexpr Type s_type = Type::BEZIER_BEZIER_TANGENT_SYMMETRIC;
    Type get_type() const override
    {
        return s_type;
    }
    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;
};

} // namespace dune3d
