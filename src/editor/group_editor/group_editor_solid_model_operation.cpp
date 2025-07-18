#include "group_editor_solid_model_operation.hpp"
#include "widgets/group_button.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"
#include "document/group/group_solid_model_operation.hpp"

namespace dune3d {

GroupEditorSolidModelOperation::GroupEditorSolidModelOperation(Core &core, const UUID &group_uu)
    : GroupEditorSolidModel(core, group_uu)
{
    add_operation_combo();

    m_argument_button = Gtk::make_managed<GroupButton>(m_core.get_current_document(), group_uu);
    m_tool_button = Gtk::make_managed<GroupButton>(m_core.get_current_document(), group_uu);

    grid_attach_label_and_widget(*this, "Argument", *m_argument_button, m_top);
    grid_attach_label_and_widget(*this, "Tool", *m_tool_button, m_top);

    auto &group = get_group();

    m_argument_button->set_group(group.m_source_group_argument);
    m_tool_button->set_group(group.m_source_group_tool);

    m_argument_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorSolidModelOperation::changed));
    m_tool_button->signal_changed().connect(sigc::mem_fun(*this, &GroupEditorSolidModelOperation::changed));

    auto swap_button = Gtk::make_managed<Gtk::Button>("Swap Argument & Tool");
    attach(*swap_button, 1, m_top++, 1, 1);
    swap_button->signal_clicked().connect([this] {
        auto &group = get_group();
        std::swap(group.m_source_group_argument, group.m_source_group_tool);
        m_argument_button->set_group(group.m_source_group_argument);
        m_tool_button->set_group(group.m_source_group_tool);
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit(CommitMode::IMMEDIATE);
    });
}

void GroupEditorSolidModelOperation::do_reload()
{
    GroupEditorSolidModel::do_reload();
    auto &group = get_group();
    m_argument_button->set_group(group.m_source_group_argument);
    m_tool_button->set_group(group.m_source_group_tool);
}

void GroupEditorSolidModelOperation::changed()
{
    auto &group = get_group();
    group.m_source_group_argument = m_argument_button->get_group();
    group.m_source_group_tool = m_tool_button->get_group();
    m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
    m_signal_changed.emit(CommitMode::IMMEDIATE);
}

GroupSolidModelOperation &GroupEditorSolidModelOperation::get_group()
{
    return m_core.get_current_document().get_group<GroupSolidModelOperation>(m_group_uu);
}

} // namespace dune3d
