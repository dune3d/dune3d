#include "tool_enter_text.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/entity/entity_text.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/text_render.hpp"
#include "tool_common_impl.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_text_window.hpp"

namespace dune3d {

ToolBase::CanBegin ToolEnterText::can_begin()
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::TEXT);
    if (!enp)
        return false;
    return enp->point == 0;
}

ToolResponse ToolEnterText::begin(const ToolArgs &args)
{
    auto enp = point_from_selection(get_doc(), m_selection, Entity::Type::TEXT);
    if (!enp)
        return ToolResponse::end();

    m_entity = &get_entity<EntityText>(enp->entity);
    m_intf.get_dialogs().show_enter_text_window("Enter text", m_entity->m_text);

    return ToolResponse();
}

ToolResponse ToolEnterText::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterTextWindow *>(args.data.get())) {
                    m_entity->m_text = d->text;
                    render_text(*m_entity, m_intf.get_pango_context(), get_doc());
                    set_current_group_generate_pending();
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
