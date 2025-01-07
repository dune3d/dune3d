#include "tool_constrain_perpendicular.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_angle.hpp"
#include "core/tool_id.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

static std::optional<std::pair<UUID, UUID>> two_lines_from_selection(const Document &doc,
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

    if (sr1.item == sr2.item)
        return {};
    if (sr1.point != 0)
        return {};
    if (sr2.point != 0)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    if (en1.of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D)
        && en2.of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

bool ToolConstrainPerpendicular::can_preview_constrain()
{
    return m_tool_id == ToolID::CONSTRAIN_PERPENDICULAR;
}

ToolBase::CanBegin ToolConstrainPerpendicular::can_begin()
{
    auto tl = two_lines_from_selection(get_doc(), m_selection);
    if (!tl.has_value())
        return false;

    if (m_tool_id == ToolID::MEASURE_ANGLE)
        return true;

    std::set<EntityAndPoint> enps = {{tl->first, 0}, {tl->second, 0}};

    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::PARALLEL, Constraint::Type::LINES_PERPENDICULAR,
                                                Constraint::Type::LINES_ANGLE);
}

ToolResponse ToolConstrainPerpendicular::begin(const ToolArgs &args)
{
    auto &doc = get_doc();
    auto tl = two_lines_from_selection(get_doc(), m_selection);
    if (!tl)
        return ToolResponse::end();

    ConstraintAngleBase *constraint = nullptr;
    if (m_tool_id == ToolID::CONSTRAIN_PERPENDICULAR) {
        constraint = &add_constraint<ConstraintLinesPerpendicular>();
    }
    else {
        auto &c = just_add_constraint<ConstraintLinesAngle>();
        constraint = &c;
        const auto l1p1 = doc.get_point({tl->first, 1});
        const auto l1p2 = doc.get_point({tl->first, 2});
        const auto l2p1 = doc.get_point({tl->second, 1});
        const auto l2p2 = doc.get_point({tl->second, 2});
        c.m_negative = (l1p1 == l2p2 || l1p2 == l2p1);
        if (m_tool_id == ToolID::MEASURE_ANGLE) {
            c.m_measurement = true;
        }
        else {
            set_current_group_solve_pending();
            c.m_modify_to_satisfy = true;
        }
    }
    constraint->m_wrkpl = get_workplane_uuid();
    constraint->m_entity1 = tl->first;
    constraint->m_entity2 = tl->second;

    return commit();
}


} // namespace dune3d
