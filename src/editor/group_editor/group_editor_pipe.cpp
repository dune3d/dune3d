#include "group_editor_pipe.hpp"
#include "util/gtk_util.hpp"
#include "document/group/group_pipe.hpp"
#include "document/entity/entity.hpp"
#include "core/core.hpp"
#include "core/tool_id.hpp"
#include <format>

namespace dune3d {

GroupEditorPipe::GroupEditorPipe(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
{
    add_operation_combo();
    m_spine_label = Gtk::make_managed<Gtk::Label>("No entities");
    m_spine_label->set_xalign(0);
    auto button = Gtk::make_managed<Gtk::Button>();
    button->set_child(*m_spine_label);
    button->signal_clicked().connect([this] { m_signal_trigger_action.emit(ToolID::SELECT_SPINE_ENTITIES); });
    grid_attach_label_and_widget(*this, "Spine", *button, m_top);
    update_label();
}

void GroupEditorPipe::do_reload()
{
    GroupEditorSweep::do_reload();
    update_label();
}

void GroupEditorPipe::update_label()
{
    auto &group = get_group();
    auto sz = group.m_spine_entities.size();
    std::string label;
    if (sz == 0)
        label = "No entities";
    else
        label = std::format("{} {}", sz, Entity::get_type_name_for_n(EntityType::INVALID, sz));
    m_spine_label->set_label(label);
}

GroupPipe &GroupEditorPipe::get_group()
{
    return m_core.get_current_document().get_group<GroupPipe>(m_group_uu);
}

} // namespace dune3d
