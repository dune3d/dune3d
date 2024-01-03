#include "constraint_point_on_line.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"

namespace dune3d {
ConstraintPointOnLine::ConstraintPointOnLine(const UUID &uu) : ConstraintPointOnLineBase(uu)
{
}

ConstraintPointOnLine::ConstraintPointOnLine(const UUID &uu, const json &j)
    : ConstraintPointOnLineBase(uu, j), m_val(j.at("val").get<double>())
{
}


json ConstraintPointOnLine::serialize() const
{
    json j = ConstraintPointOnLineBase::serialize();
    j["val"] = m_val;
    return j;
}

std::unique_ptr<Constraint> ConstraintPointOnLine::clone() const
{
    return std::make_unique<ConstraintPointOnLine>(*this);
}

void ConstraintPointOnLine::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

} // namespace dune3d
