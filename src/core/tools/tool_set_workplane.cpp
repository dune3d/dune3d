#include "tool_set_workplane.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group.hpp"
#include "core/tool_id.hpp"
#include "tool_common_impl.hpp"
#include "util/selection_util.hpp"

namespace dune3d {

bool ToolSetWorkplane::is_specific()
{
    return m_tool_id == ToolID::SET_WORKPLANE;
}

ToolBase::CanBegin ToolSetWorkplane::can_begin()
{
    if (m_tool_id == ToolID::SET_WORKPLANE) {
        auto wrkpl = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
        return wrkpl.has_value();
    }
    else { // UNSET_WORKPLANE
        return get_group().m_active_wrkpl != UUID();
    }
}

ToolResponse ToolSetWorkplane::begin(const ToolArgs &args)
{
    if (m_tool_id == ToolID::SET_WORKPLANE) {
        auto wrkpl = point_from_selection(get_doc(), m_selection, Entity::Type::WORKPLANE);
        get_group().m_active_wrkpl = wrkpl->entity;
    }
    else {
        get_group().m_active_wrkpl = UUID();
    }
    // array groups do different things depending on whether there's a workplane or not
    get_doc().set_group_generate_pending(m_core.get_current_group());
    return ToolResponse::commit();
}


ToolResponse ToolSetWorkplane::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
