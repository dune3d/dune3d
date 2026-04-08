#include "group_editor_chamfer.hpp"
#include "widgets/spin_button_dim.hpp"
#include "document/group/group_chamfer.hpp"
#include "util/gtk_util.hpp"
#include "core/core.hpp"

namespace dune3d {

GroupEditorChamfer::GroupEditorChamfer(Core &core, const UUID &group_uu) : GroupEditorLocalOperation(core, group_uu)
{
    construct();
}

std::string GroupEditorChamfer::get_radius_label() const
{
    return "Radius 1";
}

void GroupEditorChamfer::construct_extra()
{
    m_radius2_sp = Gtk::make_managed<SpinButtonDim>();
    m_radius2_sp->set_range(0, 1000);
    m_radius2_sp->set_hexpand(true);
    m_radius2_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_cb_sg->add_widget(*m_radius2_cb);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    box->append(*m_radius2_cb);
    box->append(*m_radius2_sp);

    auto &group = get_group();
    if (group.m_radius2.has_value()) {
        m_radius2_sp->set_value(*group.m_radius2);
        m_radius2_cb->set_active(true);
    }
    else {
        m_radius2_sp->set_value(group.m_radius);
        m_radius2_sp->set_sensitive(false);
        m_radius2_cb->set_active(false);
    }


    grid_attach_label_and_widget(*this, "Radius 2", *box, m_top).add_css_class("tnum");
    m_radius2_cb->signal_toggled().connect([this] {
        if (update_radius2())
            m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    connect_spinbutton(*m_radius2_sp, sigc::mem_fun(*this, &GroupEditorChamfer::update_radius2));
}

Gtk::Widget &GroupEditorChamfer::pack_radius_sp(Gtk::Widget &w)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
    auto la = Gtk::make_managed<Gtk::Label>();
    m_cb_sg->add_widget(*la);
    box->append(*la);
    box->append(w);
    return *box;
}

GroupChamfer &GroupEditorChamfer::get_group()
{
    return m_core.get_current_document().get_group<GroupChamfer>(m_group_uu);
}

bool GroupEditorChamfer::update_radius2()
{
    if (is_reloading())
        return false;
    auto &group = get_group();
    if (!group.m_radius2.has_value() && !m_radius2_cb->get_active()) // no radius2
        return false;
    if (group.m_radius2.has_value() && m_radius2_cb->get_active() && *group.m_radius2 == m_radius2_sp->get_value())
        return false;
    if (m_radius2_cb->get_active())
        group.m_radius2 = m_radius2_sp->get_value();
    else
        group.m_radius2.reset();

    m_core.get_current_document().set_group_update_solid_model_pending(get_group().m_uuid);
    return true;
}

void GroupEditorChamfer::do_reload()
{
    GroupEditorLocalOperation::do_reload();
    auto &group = get_group();
    if (group.m_radius2.has_value()) {
        m_radius2_sp->set_value(*group.m_radius2);
        m_radius2_sp->set_sensitive(true);
        m_radius2_cb->set_active(true);
    }
    else {
        m_radius2_sp->set_value(group.m_radius);
        m_radius2_sp->set_sensitive(false);
        m_radius2_cb->set_active(false);
    }
}

} // namespace dune3d
