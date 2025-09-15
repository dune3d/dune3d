#include "constraint_bezier_arc_same_curvature.hpp"
#include "nlohmann/json.hpp"
#include "util/json_util.hpp"
#include "constraint_visitor.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {
std::unique_ptr<Constraint> ConstraintBezierArcSameCurvature::clone() const
{
    return std::make_unique<ConstraintBezierArcSameCurvature>(*this);
}

void ConstraintBezierArcSameCurvature::accept(ConstraintVisitor &visitor) const
{
    visitor.visit(*this);
}

const EntityAndPoint &ConstraintBezierArcSameCurvature::get_bezier(const Document &doc) const
{
    auto &en1 = doc.get_entity(m_arc1.entity);
    if (en1.of_type(Entity::Type::BEZIER_2D))
        return m_arc1;
    else
        return m_arc2;
}

const EntityAndPoint &ConstraintBezierArcSameCurvature::get_arc(const Document &doc) const
{
    auto &en1 = doc.get_entity(m_arc1.entity);
    if (en1.of_type(Entity::Type::ARC_2D))
        return m_arc1;
    else
        return m_arc2;
}

} // namespace dune3d
