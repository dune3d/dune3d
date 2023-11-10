#include "editor.hpp"
#include "workspace_browser.hpp"
#include "dune3d_appwindow.hpp"
#include "widgets/constraints_box.hpp"
#include "document/group/all_groups.hpp"
#include "document/entity/entity_workplane.hpp"
#include "util/selection_util.hpp"
#include "canvas/canvas.hpp"
#include <iostream>

namespace dune3d {

void Editor::init_workspace_browser()
{
    m_workspace_browser = Gtk::make_managed<WorkspaceBrowser>(m_core);
    m_workspace_browser->signal_close_document().connect(
            [this](const UUID &doc_uu) { close_document(doc_uu, nullptr, nullptr); });
    m_workspace_browser->update_documents(m_document_view);

    m_workspace_browser->signal_group_selected().connect(
            sigc::mem_fun(*this, &Editor::on_workspace_browser_group_selected));
    m_workspace_browser->signal_add_group().connect(sigc::mem_fun(*this, &Editor::on_workspace_browser_add_group));
    m_workspace_browser->signal_delete_current_group().connect(
            sigc::mem_fun(*this, &Editor::on_workspace_browser_delete_current_group));
    m_workspace_browser->signal_move_group().connect(sigc::mem_fun(*this, &Editor::on_workspace_browser_move_group));
    m_workspace_browser->signal_group_checked().connect(
            sigc::mem_fun(*this, &Editor::on_workspace_browser_group_checked));
    m_workspace_browser->signal_body_checked().connect(
            sigc::mem_fun(*this, &Editor::on_workspace_browser_body_checked));
    m_workspace_browser->signal_body_solid_model_checked().connect(
            sigc::mem_fun(*this, &Editor::on_workspace_browser_body_solid_model_checked));


    m_workspace_browser->set_sensitive(m_core.has_documents());


    m_core.signal_rebuilt().connect([this] {
        Glib::signal_idle().connect_once([this] { m_workspace_browser->update_documents(m_document_view); });
    });

    m_win.get_left_bar().set_start_child(*m_workspace_browser);
}

void Editor::on_workspace_browser_group_selected(const UUID &uu_doc, const UUID &uu_group)
{
    if (m_core.tool_is_active())
        return;
    if (m_core.get_current_idocument_info().get_uuid() == uu_doc) {
        if (m_core.get_current_group() == uu_group)
            return;
        m_core.set_current_group(uu_group);
        m_workspace_browser->update_current_group(m_document_view);
        canvas_update_keep_selection();
        update_workplane_label();
        m_constraints_box->update();
        update_group_editor();
        update_action_sensitivity();
    }
}

void Editor::on_workspace_browser_add_group(Group::Type group_type)
{
    if (m_core.tool_is_active())
        return;
    auto &doc = m_core.get_current_document();
    auto &current_group = doc.get_group(m_core.get_current_group());
    Group *new_group = nullptr;
    if (group_type == Group::Type::SKETCH) {
        auto &group = doc.insert_group<GroupSketch>(UUID::random(), current_group.m_uuid);
        new_group = &group;
        group.m_name = "Sketch";
    }
    else if (group_type == Group::Type::EXTRUDE) {
        if (!current_group.m_active_wrkpl)
            return;
        auto &group = doc.insert_group<GroupExtrude>(UUID::random(), current_group.m_uuid);
        new_group = &group;
        group.m_name = "Extrude";
        group.m_wrkpl = current_group.m_active_wrkpl;
        group.m_dvec = doc.get_entity<EntityWorkplane>(group.m_wrkpl).get_normal();
        group.m_source_group = current_group.m_uuid;
    }
    else if (group_type == Group::Type::LATHE) {
        if (!current_group.m_active_wrkpl)
            return;
        auto sel = get_canvas().get_selection();
        std::optional<UUID> wrkpl = entity_from_selection(doc, sel, Entity::Type::WORKPLANE);
        std::optional<LineAndPoint> line_and_point =
                line_and_point_from_selection(doc, sel, LineAndPoint::AllowSameEntity::YES);
        if (!wrkpl && !line_and_point)
            return;

        auto &group = doc.insert_group<GroupLathe>(UUID::random(), current_group.m_uuid);
        new_group = &group;
        group.m_name = "Lathe";
        group.m_wrkpl = current_group.m_active_wrkpl;
        group.m_source_group = current_group.m_uuid;
        if (wrkpl) {
            group.m_origin = *wrkpl;
            group.m_origin_point = 1;
            group.m_normal = *wrkpl;
        }
        else {
            assert(line_and_point.has_value());
            group.m_origin = line_and_point->point;
            group.m_origin_point = line_and_point->point_point;
            group.m_normal = line_and_point->line;
        }
    }
    else if (group_type == Group::Type::FILLET) {
        auto &group = doc.insert_group<GroupFillet>(UUID::random(), current_group.m_uuid);
        new_group = &group;
        group.m_name = "Fillet";
    }
    else if (group_type == Group::Type::CHAMFER) {
        auto &group = doc.insert_group<GroupChamfer>(UUID::random(), current_group.m_uuid);
        new_group = &group;
        group.m_name = "Chamfer";
        m_core.set_needs_save();
    }
    if (new_group) {
        doc.set_group_generate_pending(new_group->m_uuid);
        m_core.set_needs_save();
        m_core.rebuild("add group");
        m_workspace_browser->update_documents(m_document_view);
        canvas_update_keep_selection();
        m_workspace_browser->select_group(new_group->m_uuid);
    }
}

void Editor::on_workspace_browser_delete_current_group()
{
    if (m_core.tool_is_active())
        return;

    auto &doc = m_core.get_current_document();

    auto &group = doc.get_group(m_core.get_current_group());
    if (group.get_type() == Group::Type::REFERENCE)
        return;

    UUID previous_group;
    previous_group = doc.get_group_rel(group.m_uuid, -1);
    if (!previous_group)
        previous_group = doc.get_group_rel(group.m_uuid, 1);

    if (!previous_group)
        return;

    {
        Document::ItemsToDelete items;
        items.groups = {group.m_uuid};
        auto exra_items = doc.get_additional_items_to_delete(items);
        items.append(exra_items);
        doc.delete_items(items);
    }

    m_core.set_current_group(previous_group);

    m_core.set_needs_save();
    m_core.rebuild("delete group");
    canvas_update_keep_selection();
    m_workspace_browser->update_documents(m_document_view);
}

void Editor::on_workspace_browser_move_group(WorkspaceBrowser::MoveGroup op)
{
    if (m_core.tool_is_active())
        return;
    auto &doc = m_core.get_current_document();

    auto &group = doc.get_group(m_core.get_current_group());
    auto groups_by_body = doc.get_groups_by_body();

    UUID group_after;
    using Op = WorkspaceBrowser::MoveGroup;
    switch (op) {
    case Op::UP:
        group_after = doc.get_group_rel(group.m_uuid, -2);
        break;
    case Op::DOWN:
        group_after = doc.get_group_rel(group.m_uuid, 1);
        break;
    case Op::END_OF_DOCUMENT:
        group_after = groups_by_body.back().groups.back()->m_uuid;
        break;
    case Op::END_OF_BODY: {
        for (auto it_body = groups_by_body.begin(); it_body != groups_by_body.end(); it_body++) {
            auto it_group = std::ranges::find(it_body->groups, &group);
            if (it_group == it_body->groups.end())
                continue;

            if (it_group == (it_body->groups.end() - 1)) {
                // is at end of body, move to end of next body
                auto it_next_body = it_body + 1;
                if (it_next_body == groups_by_body.end()) {
                    it_next_body = it_body;
                }
                group_after = it_next_body->groups.back()->m_uuid;
            }
            else {
                group_after = it_body->groups.back()->m_uuid;
            }
        }

    } break;
    }
    if (!group_after)
        return;

    std::cout << "move after " << doc.get_group(group_after).m_name << std::endl;

    if (!doc.reorder_group(group.m_uuid, group_after))
        return;
    m_core.set_needs_save();
    m_core.rebuild("reorder_group");
    canvas_update_keep_selection();
    m_workspace_browser->update_documents(m_document_view);
}

void Editor::on_workspace_browser_group_checked(const UUID &uu_doc, const UUID &uu_group, bool checked)
{
    m_document_view.m_group_views[uu_group].m_visible = checked;
    m_workspace_browser->update_current_group(m_document_view);
    canvas_update_keep_selection();
}

void Editor::on_workspace_browser_body_checked(const UUID &uu_doc, const UUID &uu_group, bool checked)
{
    m_document_view.m_body_views[uu_group].m_visible = checked;
    m_workspace_browser->update_current_group(m_document_view);
    canvas_update_keep_selection();
}

void Editor::on_workspace_browser_body_solid_model_checked(const UUID &uu_doc, const UUID &uu_group, bool checked)
{

    m_document_view.m_body_views[uu_group].m_solid_model_visible = checked;
    m_workspace_browser->update_current_group(m_document_view);
    canvas_update_keep_selection();
}

} // namespace dune3d
