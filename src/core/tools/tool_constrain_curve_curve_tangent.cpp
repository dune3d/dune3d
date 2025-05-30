#include "tool_constrain_curve_curve_tangent.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_bezier_bezier_tangent_symmetric.hpp"
#include "document/constraint/constraint_points_coincident.hpp"
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {
namespace {
struct Curves {
    EntityAndPoint curve1;
    EntityAndPoint curve2;
    UUID coincident_constraint;
};
} // namespace

static std::optional<Curves> curves_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    auto cc = constraint_points_coincident_from_selection(doc, sel, {Entity::Type::ARC_2D, Entity::Type::BEZIER_2D});
    if (!cc)
        return {};

    return {{cc->m_entity1, cc->m_entity2, cc->m_uuid}};
}

ToolBase::CanBegin ToolConstrainCurveCurveTangent::can_begin()
{
    auto curves = curves_from_selection(get_doc(), m_selection);
    if (!curves.has_value())
        return false;

    const auto &en1 = get_entity(curves->curve1.entity);
    const auto &en2 = get_entity(curves->curve2.entity);

    if (!any_entity_from_current_group(en1, en2))
        return false;

    if (m_tool_id == ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC) {
        if (!(en1.of_type(Entity::Type::BEZIER_2D) && en2.of_type(Entity::Type::BEZIER_2D)))
            return false;
    }
    else {
        if (!(en1.of_type(Entity::Type::BEZIER_2D, Entity::Type::ARC_2D)
              && en2.of_type(Entity::Type::BEZIER_2D, Entity::Type::ARC_2D)))
            return false;
    }

    return !has_constraint_of_type({curves->curve1, curves->curve2}, Constraint::Type::BEZIER_BEZIER_TANGENT_SYMMETRIC,
                                   Constraint::Type::ARC_ARC_TANGENT);
}

ToolResponse ToolConstrainCurveCurveTangent::begin(const ToolArgs &args)
{
    auto curves = curves_from_selection(get_doc(), m_selection);
    if (!curves)
        return ToolResponse::end();

    {
        const auto &curve1 = get_entity<IEntityInWorkplane>(curves->curve1.entity);
        const auto &curve2 = get_entity<IEntityInWorkplane>(curves->curve2.entity);
        if (curve1.get_workplane() != curve2.get_workplane()) {
            m_intf.tool_bar_flash("curves must be in the same workplane");
            return ToolResponse::end();
        }

        auto &cc = get_doc().get_constraint<ConstraintPointsCoincident>(curves->coincident_constraint);
        if (cc.m_wrkpl != curve1.get_workplane()) {
            m_intf.tool_bar_flash("curves must be coincident in their workplane");
            return ToolResponse::end();
        }
    }

    ConstraintArcArcTangent &constraint = m_tool_id == ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC
                                                  ? add_constraint<ConstraintBezierBezierTangentSymmetric>()
                                                  : add_constraint<ConstraintArcArcTangent>();
    constraint.m_arc1 = curves->curve1;
    constraint.m_arc2 = curves->curve2;

    return commit();
}

} // namespace dune3d
