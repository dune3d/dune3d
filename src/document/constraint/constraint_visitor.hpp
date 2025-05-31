#pragma once
#include "all_constraints_fwd.hpp"

namespace dune3d {
class ConstraintVisitor {
public:
    virtual void visit(const ConstraintPointsCoincident &constraint) = 0;
    virtual void visit(const ConstraintParallel &constraint) = 0;
    virtual void visit(const ConstraintPointOnLine &constraint) = 0;
    virtual void visit(const ConstraintPointOnCircle &constraint) = 0;
    virtual void visit(const ConstraintEqualLength &constraint) = 0;
    virtual void visit(const ConstraintEqualRadius &constraint) = 0;
    virtual void visit(const ConstraintSameOrientation &constraint) = 0;
    virtual void visit(const ConstraintHV &constraint) = 0;
    virtual void visit(const ConstraintPointDistance &constraint) = 0;
    virtual void visit(const ConstraintPointDistanceHV &constraint) = 0;
    virtual void visit(const ConstraintWorkplaneNormal &constraint) = 0;
    virtual void visit(const ConstraintMidpoint &constraint) = 0;
    virtual void visit(const ConstraintDiameterRadius &constraint) = 0;
    virtual void visit(const ConstraintArcLineTangent &constraint) = 0;
    virtual void visit(const ConstraintArcArcTangent &constraint) = 0;
    virtual void visit(const ConstraintLinePointsPerpendicular &constraint) = 0;
    virtual void visit(const ConstraintLinesPerpendicular &constraint) = 0;
    virtual void visit(const ConstraintLinesAngle &constraint) = 0;
    virtual void visit(const ConstraintPointInPlane &constraint) = 0;
    virtual void visit(const ConstraintPointLineDistance &constraint) = 0;
    virtual void visit(const ConstraintPointPlaneDistance &constraint) = 0;
    virtual void visit(const ConstraintLockRotation &constraint) = 0;
    virtual void visit(const ConstraintPointInWorkplane &constraint) = 0;
    virtual void visit(const ConstraintSymmetricHV &constraint) = 0;
    virtual void visit(const ConstraintSymmetricLine &constraint) = 0;
    virtual void visit(const ConstraintPointDistanceAligned &constraint) = 0;
    virtual void visit(const ConstraintBezierLineTangent &constraint) = 0;
    virtual void visit(const ConstraintBezierBezierTangentSymmetric &constraint) = 0;
    virtual void visit(const ConstraintPointOnBezier &constraint) = 0;
    virtual void visit(const ConstraintBezierBezierSameCurvature &constraint) = 0;
};
} // namespace dune3d
