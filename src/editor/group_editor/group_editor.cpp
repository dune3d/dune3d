#include "group_editor.hpp"
#include "document/group/group.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

void GroupEditor::update_name()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    const std::string new_name = m_name_entry->get_text();
    if (group.m_name != new_name) {
        group.m_name = new_name;
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }
}

void GroupEditor::update_body_name()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    if (!group.m_body)
        return;
    auto &body = *group.m_body;
    const std::string new_name = m_body_entry->get_text();
    if (body.m_name != new_name) {
        body.m_name = new_name;
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    }
}

GroupEditor::GroupEditor(Core &core, const UUID &group_uu) : m_core(core), m_group_uu(group_uu)
{
    add_css_class("group_editor");
    set_valign(Gtk::Align::START);
    set_row_spacing(2);
    set_column_spacing(5);
    set_margin(5);
    set_row_homogeneous(true);
    m_type_label = Gtk::manage(new Gtk::Label);
    m_type_label->set_xalign(0);
    auto &group = m_core.get_current_document().get_group(group_uu);
    m_type_label->set_text(group.get_type_name());
    grid_attach_label_and_widget(*this, "Type", *m_type_label, m_top);

    m_name_entry = Gtk::make_managed<Gtk::Entry>();
    m_name_entry->set_hexpand(true);
    m_name_entry->set_text(group.m_name);
    grid_attach_label_and_widget(*this, "Name", *m_name_entry, m_top);
    connect_entry(*m_name_entry, sigc::mem_fun(*this, &GroupEditor::update_name));

    m_body_entry = Gtk::make_managed<Gtk::Entry>();
    if (group.m_body.has_value())
        m_body_entry->set_text(group.m_body->m_name);
    m_body_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_body_cb->set_active(group.m_body.has_value());
    if (&group == core.get_current_document().get_groups_sorted().front())
        m_body_cb->set_sensitive(false);
    m_body_entry->set_sensitive(group.m_body.has_value());

    m_body_color_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_body_color_cb->set_sensitive(group.m_body.has_value());
    m_body_color_cb->set_margin_start(5);
    {
        const bool has_color = group.m_body.has_value() && group.m_body->m_color.has_value();
        m_body_color_cb->set_active(has_color);
        m_body_color_button = Gtk::make_managed<Gtk::ColorDialogButton>();
        {
            auto dia = Gtk::ColorDialog::create();
            dia->set_with_alpha(false);
            m_body_color_button->set_dialog(dia);
        }
        if (has_color) {
            m_body_color_button->set_rgba(rgba_from_color(*group.m_body->m_color));
        }
        else {
            m_body_color_button->set_sensitive(false);
        }
    }
    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
        box->append(*m_body_cb);
        box->append(*m_body_entry);
        box->append(*m_body_color_cb);
        box->append(*m_body_color_button);
        grid_attach_label_and_widget(*this, "Body", *box, m_top);
    }

    connect_entry(*m_body_entry, sigc::mem_fun(*this, &GroupEditor::update_body_name));

    m_body_cb->signal_toggled().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (m_body_cb->get_active())
            group.m_body.emplace();
        else
            group.m_body.reset();
        m_body_entry->set_sensitive(group.m_body.has_value());
        m_body_color_cb->set_sensitive(group.m_body.has_value());
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });

    m_body_color_cb->signal_toggled().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (!group.m_body)
            return;
        const bool has_color = m_body_color_cb->get_active();
        if (has_color)
            group.m_body->m_color = color_from_rgba(m_body_color_button->get_rgba());
        else
            group.m_body->m_color.reset();
        m_body_color_button->set_sensitive(has_color);

        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    m_body_color_button->property_rgba().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (!group.m_body)
            return;
        if (!group.m_body->m_color)
            return;
        group.m_body->m_color = color_from_rgba(m_body_color_button->get_rgba());

        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

void GroupEditor::do_reload()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    m_name_entry->set_text(group.m_name);
    if (group.m_body.has_value()) {
        m_body_entry->set_text(group.m_body->m_name);
        m_body_color_cb->set_active(group.m_body->m_color.has_value());
        if (group.m_body->m_color.has_value())
            m_body_color_button->set_rgba(rgba_from_color(*group.m_body->m_color));
        m_body_color_button->set_sensitive(group.m_body->m_color.has_value());
    }
    m_body_cb->set_active(group.m_body.has_value());
    m_body_color_cb->set_active(group.m_body.has_value() && group.m_body->m_color.has_value());
}

void GroupEditor::reload()
{
    m_reloading = true;
    do_reload();
    m_reloading = false;
}

GroupEditor::~GroupEditor()
{
}

} // namespace dune3d
