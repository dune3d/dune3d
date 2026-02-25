#include "group_editor_local_operation.hpp"
#include "widgets/spin_button_dim.hpp"
#include "document/group/group_local_operation.hpp"
#include "util/gtk_util.hpp"
#include "core/tool_id.hpp"
#include "core/core.hpp"

namespace dune3d {

void GroupEditorLocalOperation::construct()
{
    m_radius_sp = Gtk::make_managed<SpinButtonDim>();
    m_radius_sp->set_range(0, 1000);
    m_radius_sp->set_hexpand(true);
    auto &group = get_group();
    m_radius_sp->set_value(group.m_radius);
    connect_spinbutton(*m_radius_sp, sigc::mem_fun(*this, &GroupEditorLocalOperation::update_radius));

    m_cb_sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);

    grid_attach_label_and_widget(*this, get_radius_label(), pack_radius_sp(*m_radius_sp), m_top).add_css_class("tnum");

    construct_extra();

    {
        auto button = Gtk::make_managed<Gtk::Button>("Select edges…");
        button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ToolID::SELECT_EDGES); });
        attach(*button, 0, m_top++, 2, 1);
    }
}

void GroupEditorLocalOperation::construct_extra()
{
}

std::string GroupEditorLocalOperation::get_radius_label() const
{
    return "Radius";
}

Gtk::Widget &GroupEditorLocalOperation::pack_radius_sp(Gtk::Widget &w)
{
    return w;
}

void GroupEditorLocalOperation::do_reload()
{
    GroupEditor::do_reload();
    auto &group = get_group();
    m_radius_sp->set_value(group.m_radius);
}

GroupLocalOperation &GroupEditorLocalOperation::get_group()
{
    return m_core.get_current_document().get_group<GroupLocalOperation>(m_group_uu);
}

bool GroupEditorLocalOperation::update_radius()
{
    if (is_reloading())
        return false;
    if (get_group().m_radius == m_radius_sp->get_value())
        return false;
    get_group().m_radius = m_radius_sp->get_value();
    m_core.get_current_document().set_group_update_solid_model_pending(get_group().m_uuid);
    return true;
}

} // namespace dune3d
