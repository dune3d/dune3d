#include "tool_common.hpp"
#include "document/entity_workplane.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {
Document &ToolCommon::get_doc()
{
    return m_core.get_current_document();
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
