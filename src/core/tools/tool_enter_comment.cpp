#include "tool_enter_comment.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint_comment.hpp"
#include "editor/editor_interface.hpp"
#include "util/selection_util.hpp"
#include "util/text_render.hpp"
#include "tool_common_impl.hpp"
#include "dialogs/dialogs.hpp"
#include "dialogs/enter_text_window.hpp"

namespace dune3d {

ConstraintComment *ToolEnterComment::get_constraint()
{
    auto filtered = filter_selection(m_selection, SelectableRef::Type::CONSTRAINT);
    if (filtered.size() != 1)
        return nullptr;

    auto &co = get_doc().get_constraint(filtered.begin()->item);
    return dynamic_cast<ConstraintComment *>(&co);
}

ToolBase::CanBegin ToolEnterComment::can_begin()
{
    return get_constraint();
}

ToolResponse ToolEnterComment::begin(const ToolArgs &args)
{
    m_constraint = get_constraint();
    if (!m_constraint)
        return ToolResponse::end();

    m_intf.get_dialogs().show_enter_text_window("Enter comment", m_constraint->m_comment);

    return ToolResponse();
}

ToolResponse ToolEnterComment::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::UPDATE) {
                if (auto d = dynamic_cast<const ToolDataEnterTextWindow *>(args.data.get())) {
                    m_constraint->m_comment = d->text;
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
