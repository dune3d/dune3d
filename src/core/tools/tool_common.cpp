#include "tool_common.hpp"
#include "document/entity/entity_workplane.hpp"
#include "tool_common_impl.hpp"
#include "document/group/group.hpp"
#include "editor/editor_interface.hpp"
#include "system/system.hpp"

namespace dune3d {
Document &ToolCommon::get_doc()
{
    return m_core.get_current_document();
}

Group &ToolCommon::get_group()
{
    return get_doc().get_group(m_core.get_current_group());
}

UUID ToolCommon::get_workplane_uuid()
{
    if (m_intf.get_use_workplane())
        return m_core.get_current_workplane();
    else
        return UUID();
}

EntityWorkplane *ToolCommon::get_workplane()
{
    auto uu = get_workplane_uuid();
    if (uu)
        return &get_entity<EntityWorkplane>(uu);
    else
        return nullptr;
}

void ToolCommon::set_current_group_solve_pending()
{
    get_doc().set_group_solve_pending(m_core.get_current_group());
}

void ToolCommon::set_current_group_generate_pending()
{
    get_doc().set_group_generate_pending(m_core.get_current_group());
}

void ToolCommon::set_current_group_update_solid_model_pending()
{
    get_doc().set_group_update_solid_model_pending(m_core.get_current_group());
}

glm::dvec3 ToolCommon::get_cursor_pos_for_workplane(const EntityWorkplane &wrkpl) const
{
    return m_intf.get_cursor_pos_for_plane(wrkpl.m_origin, wrkpl.get_normal_vector());
}

bool ToolCommon::current_group_has_redundant_constraints()
{
    System sys{get_doc(), m_core.get_current_group()};
    const auto result = sys.solve();
    return result.result != SolveResult::OKAY;
}

void ToolCommon::set_first_update_group_current()
{
    m_intf.set_first_update_group(m_core.get_current_group());
}

} // namespace dune3d
