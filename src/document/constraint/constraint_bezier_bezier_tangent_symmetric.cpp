#include "constraint_bezier_bezier_tangent_symmetric.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
std::unique_ptr<Constraint> ConstraintBezierBezierTangentSymmetric::clone() const
{
    return std::make_unique<ConstraintBezierBezierTangentSymmetric>(*this);
}

void ConstraintBezierBezierTangentSymmetric::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}


} // namespace dune3d
