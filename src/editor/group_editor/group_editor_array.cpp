#include "group_editor_array.hpp"
#include "util/gtk_util.hpp"
#include "document/group/group_array.hpp"
#include "core/core.hpp"


namespace dune3d {

GroupEditorArray::GroupEditorArray(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
{
    auto &group = get_group();

    m_sp_count = Gtk::make_managed<Gtk::SpinButton>();
    m_sp_count->set_range(1, 100);
    m_sp_count->set_increments(1, 10);
    grid_attach_label_and_widget(*this, "Count", *m_sp_count, m_top);
    m_sp_count->set_value(group.m_count);
    connect_spinbutton(*m_sp_count, sigc::mem_fun(*this, &GroupEditorArray::update_count));


    auto items = Gtk::StringList::create();
    items->append("Original");
    items->append("First copy");
    items->append("Arbitrary");

    m_offset_combo = Gtk::make_managed<Gtk::DropDown>(items);
    m_offset_combo->set_selected(static_cast<guint>(group.m_offset));
    m_offset_combo->property_selected().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        group.m_offset = static_cast<GroupArray::Offset>(m_offset_combo->get_selected());
        m_core.get_current_document().set_group_generate_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    grid_attach_label_and_widget(*this, "Offset", *m_offset_combo, m_top);
}

void GroupEditorArray::do_reload()
{
    GroupEditor::do_reload();
    auto &group = get_group();
    m_sp_count->set_value(group.m_count);
    m_offset_combo->set_selected(static_cast<guint>(group.m_offset));
}

GroupArray &GroupEditorArray::get_group()
{
    return m_core.get_current_document().get_group<GroupArray>(m_group_uu);
}

bool GroupEditorArray::update_count()
{
    if (is_reloading())
        return false;
    if (static_cast<int>(get_group().m_count) == m_sp_count->get_value_as_int())
        return false;

    get_group().m_count = m_sp_count->get_value_as_int();
    m_core.get_current_document().set_group_generate_pending(get_group().m_uuid);
    return true;
}

} // namespace dune3d
