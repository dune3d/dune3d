#pragma once
#include "constraint_arc_arc_tangent.hpp"

namespace dune3d {

class Entity;

class ConstraintBezierBezierSameCurvature : public ConstraintArcArcTangent {
public:
    explicit ConstraintBezierBezierSameCurvature(const UUID &uu);
    explicit ConstraintBezierBezierSameCurvature(const UUID &uu, const json &j);
    static constexpr Type s_type = Type::BEZIER_BEZIER_SAME_CURVATURE;
    Type get_type() const override
    {
        return s_type;
    }
    std::unique_ptr<Constraint> clone() const override;
    void accept(ConstraintVisitor &visitor) const override;

    json serialize() const override;

    double m_beta1 = 1;
    double m_beta2 = 1;
};

} // namespace dune3d
