#include "tool_constrain_curve_curve_tangent.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_bezier_bezier_tangent_symmetric.hpp"
#include "document/constraint/constraint_bezier_bezier_same_curvature.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

static auto curves_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    return joint_from_selection(doc, sel, {Entity::Type::ARC_2D, Entity::Type::BEZIER_2D});
}

ToolBase::CanBegin ToolConstrainCurveCurveTangent::can_begin()
{
    auto curves = curves_from_selection(get_doc(), m_selection);
    if (!curves.has_value())
        return false;

    const auto &en1 = get_entity(curves->point1.entity);
    const auto &en2 = get_entity(curves->point2.entity);

    if (!any_entity_from_current_group(en1, en2))
        return false;

    if (any_of(m_tool_id, ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC,
               ToolID::CONSTRAIN_BEZIER_BEZIER_SAME_CURVATURE)) {
        if (!(en1.of_type(Entity::Type::BEZIER_2D) && en2.of_type(Entity::Type::BEZIER_2D)))
            return false;
    }
    else {
        if (!(en1.of_type(Entity::Type::BEZIER_2D, Entity::Type::ARC_2D)
              && en2.of_type(Entity::Type::BEZIER_2D, Entity::Type::ARC_2D)))
            return false;
    }

    return !has_constraint_of_type(curves->get_enps(), Constraint::Type::BEZIER_BEZIER_TANGENT_SYMMETRIC,
                                   Constraint::Type::ARC_ARC_TANGENT, Constraint::Type::BEZIER_BEZIER_SAME_CURVATURE);
}

ToolResponse ToolConstrainCurveCurveTangent::begin(const ToolArgs &args)
{
    auto curves = curves_from_selection(get_doc(), m_selection);
    if (!curves)
        return ToolResponse::end();

    auto &constraint = [this]() -> ConstraintArcArcTangent & {
        switch (m_tool_id) {
        case ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC:
            return add_constraint<ConstraintBezierBezierTangentSymmetric>();
        case ToolID::CONSTRAIN_CURVE_CURVE_TANGENT:
            return add_constraint<ConstraintArcArcTangent>();
        case ToolID::CONSTRAIN_BEZIER_BEZIER_SAME_CURVATURE:
            return add_constraint<ConstraintBezierBezierSameCurvature>();
        default:
            throw std::runtime_error("unsupported tool");
        }
    }();

    constraint.m_arc1 = curves->point1;
    constraint.m_arc2 = curves->point2;

    return commit();
}

} // namespace dune3d
