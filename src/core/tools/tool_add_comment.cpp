#include "tool_add_comment.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/entity/ientity_in_workplane.hpp"
#include "document/constraint/constraint_comment.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"
#include "util/selection_util.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_text_window.hpp"
#include "core/tool_data_window.hpp"

namespace dune3d {

ToolBase::CanBegin ToolAddComment::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection);
    return enp.has_value();
}

ToolResponse ToolAddComment::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection);
    if (!enp)
        return {};

    m_constraint = &just_add_constraint<ConstraintComment>();

    m_constraint->m_point = *enp;
    const auto &entity = get_entity(enp->entity);
    if (auto en_wrkpl = dynamic_cast<const IEntityInWorkplane *>(&entity))
        m_constraint->m_wrkpl = en_wrkpl->get_workplane();
    else if (entity.of_type(EntityType::WORKPLANE))
        m_constraint->m_wrkpl = entity.m_uuid;
    if (m_constraint->m_wrkpl)
        m_wrkpl = &get_doc().get_entity<EntityWorkplane>(m_constraint->m_wrkpl);

    m_constraint->m_comment = "comment";


    return ToolResponse();
}

ToolResponse ToolAddComment::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE && !m_have_win) {

        glm::dvec3 p;
        if (m_wrkpl)
            p = get_cursor_pos_for_workplane(*m_wrkpl);
        else
            p = m_intf.get_cursor_pos_for_plane(m_constraint->get_origin(get_doc()), m_intf.get_cam_normal());
        m_constraint->set_offset(p - m_constraint->get_origin(get_doc()));

        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {

            m_intf.get_dialogs().show_enter_text_window("Enter comment", "");

            m_have_win = true;
        } break;

        case InToolActionID::CANCEL:
        case InToolActionID::RMB:
            return ToolResponse::revert();

        default:;
        }
    }
    else if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterTextWindow *>(args.data.get())) {
                    m_constraint->m_comment = d->text;
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                m_constraint->m_group = get_entity(m_constraint->m_point.entity).m_group;
                return commit();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
        }
    }
    return ToolResponse();
}

} // namespace dune3d
