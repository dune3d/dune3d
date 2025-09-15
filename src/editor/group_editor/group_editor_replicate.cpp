#include "group_editor_replicate.hpp"
#include "document/group/group_replicate.hpp"
#include "widgets/group_button.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

void GroupEditorReplicate::add_source_widgets()
{
    GroupEditorSolidModel::add_operation_combo();

    auto &group = get_group();
    auto items = Gtk::StringList::create();
    items->append("Single group");
    items->append("Body up to group");
    items->append("Group range");

    m_sources_combo = Gtk::make_managed<Gtk::DropDown>(items);
    m_sources_combo->set_selected(static_cast<guint>(group.m_sources));
    m_sources_combo->property_selected().signal_changed().connect([this] {
        if (is_reloading())
            return;
        auto &group = get_group();
        group.m_sources = static_cast<GroupReplicate::Sources>(m_sources_combo->get_selected());
        m_core.get_current_document().set_group_generate_pending(m_group_uu);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
    grid_attach_label_and_widget(*this, "Sources", *m_sources_combo, m_top);

    auto &doc = m_core.get_current_document();
    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 2);
        m_source_group_start_button = Gtk::make_managed<GroupButton>(doc, group.m_uuid, GroupsFilter::ALL);
        m_source_group_start_button->set_group(group.m_source_group_start);
        m_source_group_start_button->set_hexpand(true);
        m_source_group_start_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorReplicate::changed));
        box->append(*m_source_group_start_button);

        m_ellipsis_label = Gtk::make_managed<Gtk::Label>("…");
        box->append(*m_ellipsis_label);

        m_source_group_button = Gtk::make_managed<GroupButton>(doc, group.m_uuid, GroupsFilter::ALL);
        m_source_group_button->set_group(group.m_source_group);
        m_source_group_button->set_hexpand(true);
        m_source_group_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorReplicate::changed));
        box->append(*m_source_group_button);

        grid_attach_label_and_widget(*this, "Source group", *box, m_top);
    }
    update_replicate();
}

GroupReplicate &GroupEditorReplicate::get_group()
{
    return m_core.get_current_document().get_group<GroupReplicate>(m_group_uu);
}

void GroupEditorReplicate::do_reload()
{
    GroupEditorSolidModel::do_reload();
    update_replicate();

    m_sources_combo->set_selected(static_cast<guint>(get_group().m_sources));
}

void GroupEditorReplicate::update_replicate()
{
    const auto srcs = get_group().m_sources;
    m_operation_combo->set_sensitive(srcs != GroupReplicate::Sources::SINGLE);
    m_source_group_start_button->set_visible(srcs == GroupReplicate::Sources::RANGE);
    m_ellipsis_label->set_visible(srcs == GroupReplicate::Sources::RANGE);
}

void GroupEditorReplicate::changed()
{
    auto &group = get_group();
    group.m_source_group = m_source_group_button->get_group();
    if (group.m_sources == GroupReplicate::Sources::RANGE)
        group.m_source_group_start = m_source_group_start_button->get_group();
    m_core.get_current_document().set_group_generate_pending(group.m_uuid);
    m_signal_changed.emit(CommitMode::IMMEDIATE);
}

}; // namespace dune3d
