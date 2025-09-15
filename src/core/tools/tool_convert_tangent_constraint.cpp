#include "tool_convert_tangent_constraint.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_arc_arc_tangent.hpp"
#include "document/constraint/constraint_bezier_bezier_tangent_symmetric.hpp"
#include "document/constraint/constraint_bezier_arc_same_curvature.hpp"
#include "document/constraint/constraint_bezier_bezier_same_curvature.hpp"

#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

ConstraintArcArcTangent *ToolConvertTangentConstraint::get_constraint()
{
    auto filtered = filter_selection(m_selection, SelectableRef::Type::CONSTRAINT);
    if (filtered.size() != 1)
        return nullptr;

    auto &co = get_doc().get_constraint(filtered.begin()->item);
    return dynamic_cast<ConstraintArcArcTangent *>(&co);
}

ToolBase::CanBegin ToolConvertTangentConstraint::can_begin()
{
    auto constraint = get_constraint();
    if (!constraint)
        return false;

    const auto &en1 = get_entity(constraint->m_arc1.entity);
    const auto &en2 = get_entity(constraint->m_arc2.entity);
    using CT = Constraint::Type;
    using ET = Entity::Type;

    switch (m_tool_id) {
    case ToolID::CONVERT_TO_SAME_CURVATURE_CONSTRAINT:
        if (constraint->of_type(CT::BEZIER_ARC_SAME_CURVATURE, CT::BEZIER_BEZIER_SAME_CURVATURE))
            return false;
        if (en1.of_type(ET::ARC_2D) && en2.of_type(ET::ARC_2D))
            return false;
        return true;

    case ToolID::CONVERT_TO_TANGENT_CONSTRAINT:
        if (constraint->of_type(CT::ARC_ARC_TANGENT))
            return false;
        return true;

    case ToolID::CONVERT_TO_TANGENT_SYMMETRIC_CONSTRAINT:
        if (constraint->of_type(CT::BEZIER_BEZIER_TANGENT_SYMMETRIC))
            return false;
        if (en1.of_type(ET::BEZIER_2D) && en2.of_type(ET::BEZIER_2D))
            return true;
        return false;

    default:
        return false;
    }

    return false;
}

ToolResponse ToolConvertTangentConstraint::begin(const ToolArgs &args)
{
    auto constraint = get_constraint();
    if (!constraint)
        return ToolResponse::end();

    const auto &en1 = get_entity(constraint->m_arc1.entity);
    const auto &en2 = get_entity(constraint->m_arc2.entity);
    using ET = Entity::Type;

    ConstraintArcArcTangent *new_constraint = nullptr;

    switch (m_tool_id) {
    case ToolID::CONVERT_TO_SAME_CURVATURE_CONSTRAINT:
        if (en1.of_type(ET::BEZIER_2D) && en2.of_type(ET::BEZIER_2D))
            new_constraint = &add_constraint<ConstraintBezierBezierSameCurvature>();
        else
            new_constraint = &add_constraint<ConstraintBezierArcSameCurvature>();
        break;

    case ToolID::CONVERT_TO_TANGENT_CONSTRAINT:
        new_constraint = &add_constraint<ConstraintArcArcTangent>();
        break;

    case ToolID::CONVERT_TO_TANGENT_SYMMETRIC_CONSTRAINT:
        new_constraint = &add_constraint<ConstraintBezierBezierTangentSymmetric>();
        break;

    default:
        return ToolResponse::end();
    }

    new_constraint->m_arc1 = constraint->m_arc1;
    new_constraint->m_arc2 = constraint->m_arc2;

    get_doc().delete_items({.constraints = {constraint->m_uuid}});

    return ToolResponse::commit();
}

ToolResponse ToolConvertTangentConstraint::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
