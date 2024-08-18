#include "tool_constrain_curve_curve_tangent.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_bezier_bezier_tangent_symmetric.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

static std::optional<std::pair<UUID, UUID>> two_arcs_from_selection(const Document &doc,
                                                                    const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    if (sr1.point != 0)
        return {};

    if (sr2.point != 0)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    if (en1.of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D)
        && en2.of_type(Entity::Type::ARC_2D, Entity::Type::BEZIER_2D))
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}


ToolBase::CanBegin ToolConstrainCurveCurveTangent::can_begin()
{
    auto bezs = two_arcs_from_selection(get_doc(), m_selection);
    if (!bezs.has_value())
        return false;
    if (m_tool_id == ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC)
        return get_entity(bezs->first).of_type(Entity::Type::BEZIER_2D)
               && get_entity(bezs->second).of_type(Entity::Type::BEZIER_2D);

    return true;
}

ToolResponse ToolConstrainCurveCurveTangent::begin(const ToolArgs &args)
{
    auto tp = two_arcs_from_selection(get_doc(), m_selection);
    if (!tp)
        return ToolResponse::end();

    auto &arc1 = get_entity(tp->first);
    auto &arc2 = get_entity(tp->second);
    if (dynamic_cast<const IEntityInWorkplane &>(arc1).get_workplane()
        != dynamic_cast<const IEntityInWorkplane &>(arc2).get_workplane())
        return ToolResponse::end();
    for (const unsigned int arc1_pt : {1, 2}) {
        for (const unsigned int arc2_pt : {1, 2}) {
            auto ap1 = arc1.get_point(arc1_pt, get_doc());
            auto ap2 = arc2.get_point(arc2_pt, get_doc());
            if (glm::length(ap1 - ap2) < 1e-6) {
                ConstraintArcArcTangent &constraint = m_tool_id == ToolID::CONSTRAIN_BEZIER_BEZIER_TANGENT_SYMMETRIC
                                                              ? add_constraint<ConstraintBezierBezierTangentSymmetric>()
                                                              : add_constraint<ConstraintArcArcTangent>();

                constraint.m_arc1 = {arc1.m_uuid, arc1_pt};
                constraint.m_arc2 = {arc2.m_uuid, arc2_pt};
                reset_selection_after_constrain();
                return ToolResponse::commit();
            }
        }
    }

    return ToolResponse::end();
}

ToolResponse ToolConstrainCurveCurveTangent::update(const ToolArgs &args)
{

    return ToolResponse();
}

} // namespace dune3d
