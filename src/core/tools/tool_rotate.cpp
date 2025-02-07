#include "tool_rotate.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/ientity_normal.hpp"
#include "util/selection_util.hpp"

#include "editor/editor_interface.hpp"
#include "dialogs/dialogs.hpp"
#include "tool_common_impl.hpp"

#include "core/tool_data_window.hpp"
#include "dialogs/rotate_window.hpp"

namespace dune3d {

ToolBase::CanBegin ToolRotate::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection);
    if (!enp)
        return false;
    auto &en = get_entity(enp->entity);
    if (!en.can_move(get_doc()))
        return false;
    return dynamic_cast<IEntityNormal *>(&en);
}

ToolResponse ToolRotate::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection);

    {
        auto &en = get_entity(enp.value().entity);
        m_entity_normal = dynamic_cast<IEntityNormal *>(&en);
        m_entity = &en;
    }
    if (!m_entity_normal)
        return ToolResponse::end();

    m_intf.get_dialogs().show_rotate_window("Enter angle", m_entity_normal->get_normal());

    return ToolResponse();
}

ToolResponse ToolRotate::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataRotateWindow *>(args.data.get())) {
                    m_entity_normal->set_normal(d->value);
                    get_doc().set_group_solve_pending(m_entity->m_group);
                    m_core.solve_current();
                }
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::CLOSE) {
                return ToolResponse::revert();
            }
        }
    }
    return ToolResponse();
}
} // namespace dune3d
