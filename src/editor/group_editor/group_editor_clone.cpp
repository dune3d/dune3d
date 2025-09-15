#include "group_editor_clone.hpp"
#include "core/core.hpp"
#include "document/group/group_clone.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

GroupEditorClone::GroupEditorClone(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
{
    auto source_group_name = m_core.get_current_document().get_group(get_group().m_source_group).m_name;
    auto source_label = Gtk::make_managed<Gtk::Label>(source_group_name);
    source_label->set_xalign(0);
    grid_attach_label_and_widget(*this, "Source group", *source_label, m_top);
}

GroupClone &GroupEditorClone::get_group()
{
    return m_core.get_current_document().get_group<GroupClone>(m_group_uu);
}

} // namespace dune3d
