#include "group_editor_solid_model.hpp"
#include "document/group/igroup_solid_model.hpp"
#include "document/group/group.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

void GroupEditorSolidModel::add_operation_combo()
{
    auto &group = get_group();
    auto items = Gtk::StringList::create();
    items->append("Union");
    items->append("Difference");
    items->append("Intersection");

    m_operation_combo = Gtk::make_managed<Gtk::DropDown>(items);
    m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
    m_operation_combo->property_selected().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        group.set_operation(static_cast<IGroupSolidModel::Operation>(m_operation_combo->get_selected()));
        m_core.get_current_document().set_group_update_solid_model_pending(m_group_uu);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    grid_attach_label_and_widget(*this, "Operation", *m_operation_combo, m_top);
}

void GroupEditorSolidModel::do_reload()
{
    GroupEditor::do_reload();
    auto &group = get_group();
    m_operation_combo->set_selected(static_cast<guint>(group.get_operation()));
}

IGroupSolidModel &GroupEditorSolidModel::get_group()
{
    return m_core.get_current_document().get_group<IGroupSolidModel>(m_group_uu);
}


} // namespace dune3d
