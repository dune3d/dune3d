#include "tool_constrain_point_in_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_point_in_workplane.hpp"
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

struct WorkplaneAndPoint {
    UUID wrkpl;
    EntityAndPoint point;
};

std::optional<WorkplaneAndPoint> workplane_and_point_from_selection(const Document &doc,
                                                                    const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto sr1 = *it++;
    auto sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    if (sr1.item == sr2.item)
        return {};

    // for an sr to be a line, it must be of LINE2D/3D and point==0
    if (doc.is_valid_point(sr2.get_entity_and_point()))
        std::swap(sr1, sr2);

    auto &sr_point = sr1;
    auto &sr_wrkpl = sr2;

    if (!doc.is_valid_point(sr_point.get_entity_and_point()))
        return {};

    if (sr_wrkpl.point != 0)
        return {};

    auto &en_wrkpl = doc.get_entity(sr_wrkpl.item);
    if (!en_wrkpl.of_type(Entity::Type::WORKPLANE))
        return {};

    auto &en_point = doc.get_entity(sr_point.item);
    if (auto en_in_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&en_point)) {
        if (en_in_wrkpl->get_workplane() == en_wrkpl.m_uuid)
            return {};
    }

    return {{sr_wrkpl.item, sr_point.get_entity_and_point()}};
}

ToolBase::CanBegin ToolConstrainPointInWorkplane::can_begin()
{
    return workplane_and_point_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainPointInWorkplane::begin(const ToolArgs &args)
{
    auto wp = workplane_and_point_from_selection(get_doc(), m_selection);

    if (!wp.has_value())
        return ToolResponse::end();

    auto &constraint = add_constraint<ConstraintPointInWorkplane>();
    constraint.m_point = wp->point;
    constraint.m_wrkpl = wp->wrkpl;

    return commit();
}

} // namespace dune3d
