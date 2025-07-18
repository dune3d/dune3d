#include "group_button.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include "select_group_dialog.hpp"

namespace dune3d {

GroupButton::GroupButton(const Document &doc, const UUID &current_group, GroupsFilter groups_filter)
    : m_doc(doc), m_groups_filter(groups_filter), m_current_group(current_group)
{
    m_label = Gtk::make_managed<Gtk::Label>();
    m_label->set_ellipsize(Pango::EllipsizeMode::END);
    m_label->set_xalign(0);
    set_child(*m_label);
    signal_clicked().connect(sigc::mem_fun(*this, &GroupButton::select_group));
}

const UUID &GroupButton::get_group() const
{
    return m_group;
}

void GroupButton::set_group(const UUID &group)
{
    m_group = group;
    update_label();
}

void GroupButton::update_label()
{
    if (m_group)
        m_label->set_text(m_doc.get_group(m_group).m_name);
    else
        m_label->set_text("(None)");
}

void GroupButton::select_group()
{
    auto dia = new SelectGroupDialog(m_doc, m_current_group, m_group, m_groups_filter);
    dia->set_transient_for(dynamic_cast<Gtk::Window &>(*get_ancestor(GTK_TYPE_WINDOW)));
    dia->present();
    dia->signal_changed().connect([this, dia] {
        auto group = dia->get_selected_group();
        m_group = group;
        update_label();
        m_signal_changed.emit();
    });
}

} // namespace dune3d
