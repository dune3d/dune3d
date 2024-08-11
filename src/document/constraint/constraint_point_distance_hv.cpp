#include "constraint_point_distance_hv.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {

std::unique_ptr<Constraint> ConstraintPointDistanceHorizontal::clone() const
{
    return std::make_unique<ConstraintPointDistanceHorizontal>(*this);
}

double ConstraintPointDistanceHorizontal::measure_distance(const Document &doc) const
{
    return get_distance_vector(doc).x;
}

double ConstraintPointDistanceVertical::measure_distance(const Document &doc) const
{
    return get_distance_vector(doc).y;
}

std::unique_ptr<Constraint> ConstraintPointDistanceVertical::clone() const
{
    return std::make_unique<ConstraintPointDistanceVertical>(*this);
}

void ConstraintPointDistanceHV::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
