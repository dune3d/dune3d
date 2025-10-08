#include "tool_constrain_length_ratio.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_length_ratio.hpp"
#include <algorithm>
#include <list>
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"
#include "tool_common_constrain_datum.hpp"
#include "core/tool_id.hpp"

namespace dune3d {
static std::list<UUID> entities_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    return entities_from_selection(doc, sel,
                                   {Entity::Type::LINE_2D, Entity::Type::LINE_3D, Entity::Type::ARC_2D,
                                    Entity::Type::ARC_3D});
}

static std::set<EntityAndPoint> enps_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    auto lines = entities_from_selection(doc, sel);
    std::set<EntityAndPoint> enps;
    for (const auto &uu : lines) {
        enps.emplace(uu, 0);
    }

    return enps;
}

ToolBase::CanBegin ToolConstrainLengthRatio::can_begin()
{
    auto enps = enps_from_selection(get_doc(), m_selection);

    if (enps.size() != 2)
        return false;

    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::LENGTH_RATIO);
}

bool ToolConstrainLengthRatio::is_force_unset_workplane()
{
    return m_tool_id == ToolID::CONSTRAIN_LENGTH_RATIO_3D;
}

bool ToolConstrainLengthRatio::constraint_is_in_workplane()
{
    return get_workplane_uuid() != UUID{};
}

ToolID ToolConstrainLengthRatio::get_force_unset_workplane_tool()
{
    auto wrkpl = get_workplane_uuid();
    if (!wrkpl)
        return ToolID::NONE;

    if (all_entities_in_current_workplane(enps_from_selection(get_doc(), m_selection)))
        return ToolID::NONE;

    return ToolID::CONSTRAIN_LENGTH_RATIO_3D;
}

ToolResponse ToolConstrainLengthRatio::begin(const ToolArgs &args)
{
    auto entities = entities_from_selection(get_doc(), m_selection);
    if (entities.size() != 2)
        return ToolResponse::end();

    auto it = entities.begin();
    const auto entity1 = *it++;
    const auto entity2 = *it;

    auto &constraint = just_add_constraint<ConstraintLengthRatio>();
    constraint.m_entity1 = entity1;
    constraint.m_entity2 = entity2;
    constraint.m_wrkpl = get_workplane_uuid();
    constraint.m_offset = {0.0, 0.0, 0.0};
    constraint.m_ratio = constraint.measure_ratio(get_doc());
    constraint.m_ratio = std::clamp(constraint.m_ratio, ConstraintLengthRatio::s_min_ratio,
                                    ConstraintLengthRatio::s_max_ratio);

    set_current_group_solve_pending();

    return prepare_interactive(constraint);
}
} // namespace dune3d
