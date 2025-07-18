#include "group_editor_revolve.hpp"
#include "document/group/group_revolve.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

GroupEditorRevolve::GroupEditorRevolve(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
{
    auto &group = get_group();
    add_operation_combo();
    {
        auto items = Gtk::StringList::create();
        items->append("Single");
        items->append("Offset");
        items->append("Offset symmetric");

        m_mode_combo = Gtk::make_managed<Gtk::DropDown>(items);
        m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
        m_mode_combo->property_selected().signal_changed().connect([this] {
            if (is_reloading())
                return;
            auto &group = get_group();
            group.m_mode = static_cast<GroupRevolve::Mode>(m_mode_combo->get_selected());
            m_core.get_current_document().set_group_generate_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        grid_attach_label_and_widget(*this, "Mode", *m_mode_combo, m_top);
    }
}

void GroupEditorRevolve::do_reload()
{
    GroupEditorSweep::do_reload();
    auto &group = get_group();
    m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
}

GroupRevolve &GroupEditorRevolve::get_group()
{
    return m_core.get_current_document().get_group<GroupRevolve>(m_group_uu);
}

} // namespace dune3d
