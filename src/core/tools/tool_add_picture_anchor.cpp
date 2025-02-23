#include "tool_add_picture_anchor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_picture.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/selection_util.hpp"
#include "util/action_label.hpp"
#include "in_tool_action/in_tool_action.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include <format>

namespace dune3d {

ToolBase::CanBegin ToolAddPictureAnchor::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::PICTURE);
    return enp.has_value() && enp->point == 0;
}

ToolResponse ToolAddPictureAnchor::begin(const ToolArgs &args)
{
    auto sr = *m_selection.begin();

    m_picture = &get_entity<EntityPicture>(sr.item);
    m_wrkpl = &get_entity<EntityWorkplane>(m_picture->m_wrkpl);

    m_selection.clear();
    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "add point");
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::CANCEL, "cancel");
        m_intf.tool_bar_set_actions(actions);
    }


    return ToolResponse();
}

ToolResponse ToolAddPictureAnchor::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        auto p = m_picture->untransform(get_cursor_pos_for_workplane(*m_wrkpl));
        m_intf.tool_bar_set_tool_tip(std::format("Position: {:.0f}, {:.0f}", p.x, p.y));
    }
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            m_picture->add_anchor(m_picture->untransform(m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl))));
            m_intf.canvas_update_from_tool();

        } break;

        case InToolActionID::RMB: {
            return ToolResponse::commit();
        }

        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}

} // namespace dune3d
