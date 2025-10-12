#include "tool_common_constrain.hpp"
#include "canvas/selection_mode.hpp"
#include "editor/editor_interface.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

bool ToolCommonConstrain::is_specific()
{
    return true;
}

bool ToolCommonConstrain::can_preview_constrain()
{
    return false;
}

ToolID ToolCommonConstrain::get_force_unset_workplane_tool()
{
    return ToolID::NONE;
}

bool ToolCommonConstrain::constraint_is_in_workplane()
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

bool ToolCommonConstrain::all_entities_in_current_workplane(const std::set<EntityAndPoint> &enps)
{
    auto wrkpl = get_workplane_uuid();
    if (!wrkpl)
        return false;
    for (const auto &enp : enps) {
        if (auto en_wrkpl = get_doc().get_entity_ptr<IEntityInWorkplane>(enp.entity)) {
            if (en_wrkpl->get_workplane() != wrkpl)
                return false;
        }
        else {
            return false;
        }
    }
    return true;
}


} // namespace dune3d
