#include "tool_common_constrain.hpp"
#include "canvas/selection_mode.hpp"
#include "editor/editor_interface.hpp"

namespace dune3d {

bool ToolCommonConstrain::is_specific()
{
    return true;
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
    m_selection.clear();
    m_intf.set_canvas_selection_mode(SelectionMode::HOVER);
}

} // namespace dune3d
