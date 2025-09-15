#include "group_editor_loft.hpp"
#include "util/gtk_util.hpp"
#include "document/group/group_loft.hpp"
#include "core/core.hpp"
#include "widgets/select_groups_dialog.hpp"

namespace dune3d {


GroupEditorLoft::GroupEditorLoft(Core &core, const UUID &group_uu) : GroupEditorSolidModel(core, group_uu)
{
    add_operation_combo();

    m_ruled_switch = Gtk::make_managed<Gtk::Switch>();
    m_ruled_switch->set_valign(Gtk::Align::CENTER);
    m_ruled_switch->set_halign(Gtk::Align::START);
    auto &group = get_group();
    m_ruled_switch->set_active(group.m_ruled);
    grid_attach_label_and_widget(*this, "Ruled", *m_ruled_switch, m_top);
    m_ruled_switch->property_active().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        group.m_ruled = m_ruled_switch->get_active();
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    m_source_groups_button = Gtk::make_managed<Gtk::Button>();
    m_source_groups_button->signal_clicked().connect(sigc::mem_fun(*this, &GroupEditorLoft::edit_source_groups));
    m_source_groups_button_label = Gtk::make_managed<Gtk::Label>();
    m_source_groups_button_label->set_ellipsize(Pango::EllipsizeMode::END);
    m_source_groups_button_label->set_xalign(0);
    m_source_groups_button->set_child(*m_source_groups_button_label);
    grid_attach_label_and_widget(*this, "Source groups", *m_source_groups_button, m_top);
    update_source_groups_label(group);
}

void GroupEditorLoft::do_reload()
{
    GroupEditorSolidModel::do_reload();
    auto &group = get_group();
    m_ruled_switch->set_active(group.m_ruled);
    update_source_groups_label(group);
}

void GroupEditorLoft::update_source_groups_label(const GroupLoft &group)
{
    std::string s;
    for (const auto &src : group.m_sources) {
        auto &gr = m_core.get_current_document().get_group(src.group);
        if (s.size())
            s += ", ";
        s += gr.m_name;
    }
    m_source_groups_button_label->set_text(s);
}

void GroupEditorLoft::edit_source_groups()
{
    std::vector<UUID> current_source_groups;
    auto &group = get_group();
    for (const auto &src : group.m_sources) {
        current_source_groups.push_back(src.group);
    }
    auto dia = SelectGroupsDialog::create(m_core.get_current_document(), group.m_uuid, current_source_groups);
    dia->set_transient_for(dynamic_cast<Gtk::Window &>(*get_ancestor(GTK_TYPE_WINDOW)));
    dia->present();
    dia->signal_changed().connect([this, dia, &group] {
        auto groups = dia->get_selected_groups();
        if (groups.size() < 2)
            return;

        decltype(group.m_sources) new_sources;
        for (const auto &uu : groups) {
            auto existing = std::ranges::find_if(group.m_sources, [uu](const auto &x) { return x.group == uu; });
            if (existing == group.m_sources.end()) {
                new_sources.emplace_back(m_core.get_current_document().get_group(uu).m_active_wrkpl, uu);
            }
            else {
                new_sources.emplace_back(*existing);
            }
        }
        group.m_sources = new_sources;


        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

GroupLoft &GroupEditorLoft::get_group()
{
    return m_core.get_current_document().get_group<GroupLoft>(m_group_uu);
}
} // namespace dune3d
