#include "group_editor_sweep.hpp"
#include "document/group/group_sweep.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

void GroupEditorSweep::add_operation_combo()
{
    GroupEditorSolidModel::add_operation_combo();

    auto source_group_name = m_core.get_current_document().get_group(get_group().m_source_group).m_name;
    auto source_label = Gtk::make_managed<Gtk::Label>(source_group_name);
    source_label->set_xalign(0);
    grid_attach_label_and_widget(*this, "Source group", *source_label, m_top);
}

GroupSweep &GroupEditorSweep::get_group()
{
    return m_core.get_current_document().get_group<GroupSweep>(m_group_uu);
}

}; // namespace dune3d
