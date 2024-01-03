#include "constraint_midpoint.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {

std::unique_ptr<Constraint> ConstraintMidpoint::clone() const
{
    return std::make_unique<ConstraintMidpoint>(*this);
}

void ConstraintMidpoint::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
