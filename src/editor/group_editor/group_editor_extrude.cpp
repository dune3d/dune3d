#include "group_editor_extrude.hpp"
#include "document/group/group_extrude.hpp"
#include "util/gtk_util.hpp"
#include "core/core.hpp"

namespace dune3d {

GroupEditorExtrude::GroupEditorExtrude(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
{
    m_normal_switch = Gtk::make_managed<Gtk::Switch>();
    m_normal_switch->set_valign(Gtk::Align::CENTER);
    m_normal_switch->set_halign(Gtk::Align::START);
    auto &group = get_group();
    m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
    grid_attach_label_and_widget(*this, "Along normal", *m_normal_switch, m_top);
    m_normal_switch->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        if (m_normal_switch->get_active())
            group.m_direction = GroupExtrude::Direction::NORMAL;
        else
            group.m_direction = GroupExtrude::Direction::ARBITRARY;
        m_core.get_current_document().set_group_solve_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
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
            group.m_mode = static_cast<GroupExtrude::Mode>(m_mode_combo->get_selected());
            m_core.get_current_document().set_group_generate_pending(group.m_uuid);
            m_signal_changed.emit(CommitMode::IMMEDIATE);
        });
        grid_attach_label_and_widget(*this, "Mode", *m_mode_combo, m_top);
    }
}

void GroupEditorExtrude::do_reload()
{
    GroupEditorSweep::do_reload();
    auto &group = get_group();
    m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
    m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
}

GroupExtrude &GroupEditorExtrude::get_group()
{
    return m_core.get_current_document().get_group<GroupExtrude>(m_group_uu);
}


} // namespace dune3d
