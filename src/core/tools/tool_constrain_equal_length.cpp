#include "tool_constrain_equal_length.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_equal_length.hpp"
#include <list>
#include "util/selection_util.hpp"
#include "tool_common_constrain_impl.hpp"
#include "core/tool_id.hpp"

namespace dune3d {
static std::list<UUID> lines_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    return entities_from_selection(doc, sel, {Entity::Type::LINE_2D, Entity::Type::LINE_3D});
}

static std::set<EntityAndPoint> enps_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    auto lines = lines_from_selection(doc, sel);
    std::set<EntityAndPoint> enps;
    for (const auto &uu : lines) {
        enps.emplace(uu, 0);
    }

    return enps;
}

ToolBase::CanBegin ToolConstrainEqualLength::can_begin()
{
    auto enps = enps_from_selection(get_doc(), m_selection);

    if (enps.size() < 2)
        return false;

    if (!any_entity_from_current_group(enps))
        return false;

    return !has_constraint_of_type_in_workplane(enps, Constraint::Type::EQUAL_LENGTH);
}

bool ToolConstrainEqualLength::is_force_unset_workplane()
{
    return m_tool_id == ToolID::CONSTRAIN_EQUAL_LENGTH_3D;
}

bool ToolConstrainEqualLength::constraint_is_in_workplane()
{
    return get_workplane_uuid() != UUID{};
}

ToolID ToolConstrainEqualLength::get_force_unset_workplane_tool()
{
    auto wrkpl = get_workplane_uuid();
    if (!wrkpl)
        return ToolID::NONE;

    if (all_entities_in_current_workplane(enps_from_selection(get_doc(), m_selection)))
        return ToolID::NONE;

    return ToolID::CONSTRAIN_EQUAL_LENGTH_3D;
}

ToolResponse ToolConstrainEqualLength::begin(const ToolArgs &args)
{
    auto lines = lines_from_selection(get_doc(), m_selection);

    auto it = lines.begin();
    auto first = it++;

    for (; it != lines.end(); it++) {
        auto &constraint = add_constraint<ConstraintEqualLength>();
        constraint.m_entity1 = *first;
        constraint.m_entity2 = *it;
        constraint.m_wrkpl = get_workplane_uuid();
    }

    return commit();
}
} // namespace dune3d
