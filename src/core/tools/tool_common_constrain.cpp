#include "tool_common_constrain.hpp"
#include "canvas/selection_mode.hpp"
#include "editor/editor_interface.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"

namespace dune3d {

bool ToolCommonConstrain::is_specific()
{
    return true;
}

bool ToolCommonConstrain::can_preview_constrain()
{
    return false;
}

ToolResponse ToolCommonConstrain::update(const ToolArgs &args)
{
    return ToolResponse();
}

ToolResponse ToolCommonConstrain::commit()
{
    reset_selection_after_constrain();
    return ToolResponse::commit();
}

void ToolCommonConstrain::reset_selection_after_constrain()
{
    if (m_is_preview)
        return;
    m_selection.clear();
    m_intf.set_canvas_selection_mode(SelectionMode::HOVER);
}

bool ToolCommonConstrain::any_entity_from_current_group(const std::set<EntityAndPoint> &enps)
{
    for (const auto &enp : enps) {
        auto &en = get_doc().get_entity(enp.entity);
        if (en.m_group == m_core.get_current_group())
            return true;
    }
    return false;
}


} // namespace dune3d
