#include "tool_move_picture_anchor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_picture.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/selection_util.hpp"
#include "in_tool_action/in_tool_action.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include <format>

namespace dune3d {

ToolBase::CanBegin ToolMovePictureAnchor::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::PICTURE);
    if (!enp)
        return false;
    auto &en = get_entity<EntityPicture>(enp->entity);

    return en.is_user_anchor(enp->point);
}

ToolResponse ToolMovePictureAnchor::begin(const ToolArgs &args)
{
    auto sr = *m_selection.begin();

    m_picture = &get_entity<EntityPicture>(sr.item);
    m_wrkpl = &get_entity<EntityWorkplane>(m_picture->m_wrkpl);
    m_anchor = sr.point;

    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    return ToolResponse();
}

ToolResponse ToolMovePictureAnchor::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        auto p = m_picture->untransform(get_cursor_pos_for_workplane(*m_wrkpl));
        m_intf.tool_bar_set_tool_tip(std::format("Position: {:.0f}, {:.0f}", p.x, p.y));
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            m_picture->update_anchor(m_anchor,
                                     m_picture->untransform(m_wrkpl->project(get_cursor_pos_for_workplane(*m_wrkpl))));
            return ToolResponse::commit();


        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::end();

        default:;
        }
    }


    return ToolResponse();
}

} // namespace dune3d
