#include "constraint_bezier_bezier_same_curvature.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraintt_impl.hpp"

namespace dune3d {
ConstraintBezierBezierSameCurvature::ConstraintBezierBezierSameCurvature(const UUID &uu) : ConstraintArcArcTangent(uu)
{
}

ConstraintBezierBezierSameCurvature::ConstraintBezierBezierSameCurvature(const UUID &uu, const json &j)
    : ConstraintArcArcTangent(uu, j), m_beta1(j.at("beta1").get<double>()), m_beta2(j.at("beta2").get<double>())
{
}

json ConstraintBezierBezierSameCurvature::serialize() const
{
    json j = ConstraintArcArcTangent::serialize();
    j["beta1"] = m_beta1;
    j["beta2"] = m_beta2;
    return j;
}

std::unique_ptr<Constraint> ConstraintBezierBezierSameCurvature::clone() const
{
    return std::make_unique<ConstraintBezierBezierSameCurvature>(*this);
}

void ConstraintBezierBezierSameCurvature::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
