#include "group_editor_mirror.hpp"
#include "document/group/group_mirror.hpp"
#include "util/gtk_util.hpp"
#include "core/core.hpp"

namespace dune3d {

GroupEditorMirror::GroupEditorMirror(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
{

    m_include_source_switch = Gtk::make_managed<Gtk::Switch>();
    m_include_source_switch->set_valign(Gtk::Align::CENTER);
    m_include_source_switch->set_halign(Gtk::Align::START);
    auto &group = get_group();
    m_include_source_switch->set_active(group.m_include_source);
    grid_attach_label_and_widget(*this, "Include source", *m_include_source_switch, m_top);
    m_include_source_switch->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        group.m_include_source = m_include_source_switch->get_active();
        m_core.get_current_document().set_group_generate_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

void GroupEditorMirror::do_reload()
{
    GroupEditor::do_reload();
    auto &group = get_group();
    m_include_source_switch->set_active(group.m_include_source);
}

GroupMirror &GroupEditorMirror::get_group()
{
    return m_core.get_current_document().get_group<GroupMirror>(m_group_uu);
}

} // namespace dune3d
