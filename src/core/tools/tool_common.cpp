#include "tool_common.hpp"
#include "document/entity/entity_workplane.hpp"
#include "tool_common_impl.hpp"
#include "document/group/group.hpp"

namespace dune3d {
Document &ToolCommon::get_doc()
{
    return m_core.get_current_document();
}

Group &ToolCommon::get_group()
{
    return get_doc().get_group(m_core.get_current_group());
}

EntityWorkplane *ToolCommon::get_workplane()
{
    auto uu = m_core.get_current_workplane();
    if (uu)
        return &get_entity<EntityWorkplane>(uu);
    else
        return nullptr;
}

} // namespace dune3d
