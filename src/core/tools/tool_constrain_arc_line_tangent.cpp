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
    UUID coincident_constraint;
};
} // namespace

static std::optional<CurveAndLine>
curve_and_line_from_selection(const Document &doc, const std::set<SelectableRef> &sel, Entity::Type curve_type)
{
    auto cc = constraint_points_coincident_from_selection(
            doc, sel, {Entity::Type::ARC_2D, Entity::Type::LINE_2D, Entity::Type::BEZIER_2D});
    if (!cc)
        return {};

    auto &en1 = doc.get_entity(cc->m_entity1.entity);
    auto &en2 = doc.get_entity(cc->m_entity2.entity);
    if (en1.of_type(curve_type) && en2.of_type(Entity::Type::LINE_2D))
        return {{cc->m_entity1, cc->m_entity2, cc->m_uuid}};
    else if (en2.of_type(curve_type) && en1.of_type(Entity::Type::LINE_2D))
        return {{cc->m_entity2, cc->m_entity1, cc->m_uuid}};

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
    {
        const auto &curve = get_entity<IEntityInWorkplane>(tp->curve.entity);
        const auto &line = get_entity<IEntityInWorkplane>(tp->line.entity);
        if (curve.get_workplane() != line.get_workplane()) {
            m_intf.tool_bar_flash("curve and line must be in the same workplane");
            return ToolResponse::end();
        }

        auto &cc = get_doc().get_constraint<ConstraintPointsCoincident>(tp->coincident_constraint);
        if (cc.m_wrkpl != curve.get_workplane()) {
            m_intf.tool_bar_flash("curve and line must be coincident in their workplane");
            return ToolResponse::end();
        }
    }

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
