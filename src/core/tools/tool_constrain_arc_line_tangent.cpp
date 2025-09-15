#include "tool_constrain_arc_line_tangent.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_arc_line_tangent.hpp"
#include "document/constraint/constraint_bezier_line_tangent.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

namespace {
struct CurveAndLine {
    EntityAndPoint curve;
    EntityAndPoint line;
};
} // namespace

static std::optional<CurveAndLine>
curve_and_line_from_selection(const Document &doc, const std::set<SelectableRef> &sel, Entity::Type curve_type)
{
    auto tp = joint_from_selection(doc, sel, {Entity::Type::ARC_2D, Entity::Type::LINE_2D, Entity::Type::BEZIER_2D});
    if (!tp)
        return {};

    auto &en1 = doc.get_entity(tp->point1.entity);
    auto &en2 = doc.get_entity(tp->point2.entity);
    if (en1.of_type(curve_type) && en2.of_type(Entity::Type::LINE_2D))
        return {{tp->point1, tp->point2}};
    else if (en2.of_type(curve_type) && en1.of_type(Entity::Type::LINE_2D))
        return {{tp->point2, tp->point1}};

    return {};
}

EntityType ToolConstrainArcLineTangent::get_curve_type() const
{
    if (m_tool_id == ToolID::CONSTRAIN_ARC_LINE_TANGENT)
        return Entity::Type::ARC_2D;
    else
        return Entity::Type::BEZIER_2D;
}

ToolBase::CanBegin ToolConstrainArcLineTangent::can_begin()
{
    auto al = curve_and_line_from_selection(get_doc(), m_selection, get_curve_type());
    if (!al.has_value())
        return false;

    if (!any_entity_from_current_group(al->curve, al->line))
        return false;

    const auto constraint_type = m_tool_id == ToolID::CONSTRAIN_ARC_LINE_TANGENT
                                         ? Constraint::Type::ARC_LINE_TANGENT
                                         : Constraint::Type::BEZIER_LINE_TANGENT;

    return !has_constraint_of_type({al->curve, {al->line.entity, 0}}, constraint_type);
}

ToolResponse ToolConstrainArcLineTangent::begin(const ToolArgs &args)
{
    auto tp = curve_and_line_from_selection(get_doc(), m_selection, get_curve_type());
    if (!tp)
        return ToolResponse::end();

    if (m_tool_id == ToolID::CONSTRAIN_ARC_LINE_TANGENT) {
        auto &constraint = add_constraint<ConstraintArcLineTangent>();
        constraint.m_arc = tp->curve;
        constraint.m_line = tp->line.entity;
    }
    else {
        auto &constraint = add_constraint<ConstraintBezierLineTangent>();
        constraint.m_bezier = tp->curve;
        constraint.m_line = tp->line.entity;
    }

    return commit();
}

} // namespace dune3d
