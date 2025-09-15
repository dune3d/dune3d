#include "group_editor_reference.hpp"
#include "util/gtk_util.hpp"
#include "document/group/group_reference.hpp"
#include "core/core.hpp"

namespace dune3d {

GroupEditorReference::GroupEditorReference(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
{
    auto &group = get_group();

    m_switch_xy = Gtk::make_managed<Gtk::Switch>();
    m_switch_xy->set_halign(Gtk::Align::START);
    m_switch_xy->set_valign(Gtk::Align::CENTER);
    grid_attach_label_and_widget(*this, "XY", *m_switch_xy, m_top);
    m_switch_xy->set_active(group.m_show_xy);
    m_switch_xy->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        get_group().m_show_xy = m_switch_xy->get_active();
        get_group().generate(m_core.get_current_document());
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    m_switch_yz = Gtk::make_managed<Gtk::Switch>();
    m_switch_yz->set_halign(Gtk::Align::START);
    m_switch_yz->set_valign(Gtk::Align::CENTER);
    grid_attach_label_and_widget(*this, "YZ", *m_switch_yz, m_top);
    m_switch_yz->set_active(group.m_show_yz);
    m_switch_yz->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        get_group().m_show_yz = m_switch_yz->get_active();
        get_group().generate(m_core.get_current_document());
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    m_switch_zx = Gtk::make_managed<Gtk::Switch>();
    m_switch_zx->set_halign(Gtk::Align::START);
    m_switch_zx->set_valign(Gtk::Align::CENTER);
    grid_attach_label_and_widget(*this, "ZX", *m_switch_zx, m_top);
    m_switch_zx->set_active(group.m_show_zx);
    m_switch_zx->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        get_group().m_show_zx = m_switch_zx->get_active();
        get_group().generate(m_core.get_current_document());
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

void GroupEditorReference::do_reload()
{
    GroupEditor::do_reload();
    auto &group = get_group();
    m_switch_xy->set_active(group.m_show_xy);
    m_switch_yz->set_active(group.m_show_yz);
    m_switch_zx->set_active(group.m_show_zx);
}

GroupReference &GroupEditorReference::get_group()
{
    return m_core.get_current_document().get_group<GroupReference>(m_group_uu);
}

} // namespace dune3d
