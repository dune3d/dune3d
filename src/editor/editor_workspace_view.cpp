#include "editor.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "workspace_browser.hpp"
#include "nlohmann/json.hpp"
#include "util/fs_util.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include <format>

namespace dune3d {

UUID Editor::create_workspace_view()
{
    auto uu = UUID::random();
    auto &wv = m_workspace_views[uu];
    for (auto doc : m_core.get_documents()) {
        wv.m_documents[doc->get_uuid()];
    }
    append_workspace_view_page(wv.m_name, uu);
    return uu;
}

UUID Editor::create_workspace_view_from_current()
{
    return duplicate_workspace_view(m_current_workspace_view);
}

UUID Editor::duplicate_workspace_view(const UUID &wv_uu)
{
    auto uu = UUID::random();
    auto &wv = m_workspace_views.emplace(uu, m_workspace_views.at(wv_uu)).first->second;
    if (wv.m_name.size())
        wv.m_name += " (Copy)";
    append_workspace_view_page(wv.m_name, uu);
    return uu;
}

void Editor::append_workspace_view_page(const std::string &name, const UUID &uu)
{
    auto &la = m_win.append_workspace_view_page(name, uu);
    la.signal_close().connect([this, uu] { close_workspace_view(uu); });
    la.signal_rename().connect([this, uu] { rename_workspace_view(uu); });
    la.signal_duplicate().connect([this, uu] { set_current_workspace_view(duplicate_workspace_view(uu)); });
    update_can_close_workspace_view_pages();
    update_workspace_view_names();
}

void Editor::close_workspace_view(const UUID &uu)
{
    if (!m_workspace_views.contains(uu))
        return;
    if (m_workspace_views.size() == 1 && m_core.has_documents())
        return;
    m_workspace_views.erase(uu);
    m_win.remove_workspace_view_page(uu);
    update_can_close_workspace_view_pages();
    update_workspace_view_names();
    if (m_workspace_views.size() == 0)
        m_current_workspace_view = {};
}

void Editor::update_can_close_workspace_view_pages()
{
    auto pages = m_win.get_workspace_notebook().get_pages();
    auto docs = m_core.get_documents();
    for (size_t i = 0; i < pages->get_n_items(); i++) {
        auto &page = dynamic_cast<Gtk::NotebookPage &>(*pages->get_object(i).get());
        auto &it = dynamic_cast<WorkspaceViewPage &>(*page.get_child());

        // we can only close a workspace view if the documents visible in it are visible in other views
        std::set<UUID> docs_in_this_view;
        {
            auto &wv = m_workspace_views.at(it.m_uuid);
            for (auto doc : docs) {
                if (wv.document_is_visible(doc->get_uuid()))
                    docs_in_this_view.insert(doc->get_uuid());
            }
        }

        bool can_close = false;
        for (const auto &doc : docs_in_this_view) {
            for (const auto &[uu, wv] : m_workspace_views) {
                if (uu == it.m_uuid) // this view
                    continue;
                if (wv.document_is_visible(doc)) {
                    can_close = true;
                    break;
                }
            }
            if (can_close)
                break;
        }


        dynamic_cast<Dune3DAppWindow::WorkspaceTabLabel &>(*m_win.get_workspace_notebook().get_tab_label(it))
                .set_can_close(can_close);
    }
}

void Editor::rename_workspace_view(const UUID &uu)
{
    if (!m_workspace_views.contains(uu))
        return;

    auto win = new RenameWindow("Rename workspace view");
    win->set_text(m_workspace_views.at(uu).m_name);
    win->set_transient_for(m_win);
    win->set_modal(true);
    win->present();
    win->signal_changed().connect([this, win, uu] {
        auto txt = win->get_text();
        m_workspace_views.at(uu).m_name = txt;
        update_workspace_view_names();
    });
}

void Editor::auto_close_workspace_views()
{
    // close a workspace view when none of the visible documents exist
    std::set<UUID> all_docs;
    for (const auto doc : m_core.get_documents()) {
        all_docs.insert(doc->get_uuid());
    }
    std::vector<UUID> views_to_close;
    for (const auto &[uu_wv, wv] : m_workspace_views) {
        const bool has_visible_documents = std::ranges::any_of(wv.m_documents, [&all_docs](auto &x) {
            return x.second.m_document_is_visible && all_docs.contains(x.first);
        });
        if (!has_visible_documents)
            views_to_close.push_back(uu_wv);
    }
    for (const auto &uu : views_to_close) {
        close_workspace_view(uu);
    }
}


void Editor::update_workspace_view_names()
{
    std::map<std::string, unsigned int> name_count;
    auto pages = m_win.get_workspace_notebook().get_pages();
    for (size_t i = 0; i < pages->get_n_items(); i++) {
        auto &page = dynamic_cast<Gtk::NotebookPage &>(*pages->get_object(i).get());
        auto &it = dynamic_cast<WorkspaceViewPage &>(*page.get_child());
        const auto &wv = m_workspace_views.at(it.m_uuid);
        auto name = wv.m_name;
        if (name.empty()) {
            for (const auto &[uu, dv] : wv.m_documents) {
                if (dv.document_is_visible() && m_core.has_document(uu)) {
                    if (name.size())
                        name += ", ";
                    name += m_core.get_idocument_info(uu).get_name();
                }
            }
        }
        if (name_count.contains(name)) {
            name += std::format(" ({})", name_count.at(name)++);
        }
        else
            name_count.emplace(name, 1);
        dynamic_cast<Dune3DAppWindow::WorkspaceTabLabel &>(*m_win.get_workspace_notebook().get_tab_label(it))
                .set_label(name);
    }
}

void Editor::set_current_workspace_view(const UUID &uu)
{
    CanvasUpdater canvas_updater{*this};

    m_current_workspace_view = uu;
    auto pages = m_win.get_workspace_notebook().get_pages();

    for (size_t i = 0; i < pages->get_n_items(); i++) {
        auto &page = dynamic_cast<Gtk::NotebookPage &>(*pages->get_object(i).get());
        auto &it = dynamic_cast<WorkspaceViewPage &>(*page.get_child());
        if (it.m_uuid == uu) {
            m_win.get_workspace_notebook().set_current_page(i);
            break;
        }
    }
    const auto &wv = m_workspace_views.at(m_current_workspace_view);
    {
        m_workspace_view_loading = true;
        auto &ca = m_win.get_canvas();

        ca.set_cam_distance(wv.m_cam_distance, Canvas::ZoomCenter::CURSOR);
        ca.set_cam_quat(wv.m_cam_quat);
        ca.set_center(wv.m_center);
        set_perspective_projection(wv.m_projection == CanvasProjection::PERSP);
        set_show_previous_construction_entities(wv.m_show_construction_entities_from_previous_groups);
        set_hide_irrelevant_workplanes(wv.m_hide_irrelevant_workplanes);
        if (wv.m_curvature_comb_scale == 0)
            m_curvature_comb_scale->set_value(m_curvature_comb_scale->get_adjustment()->get_lower());
        else
            m_curvature_comb_scale->set_value(log10(wv.m_curvature_comb_scale));
        update_view_hints();
        m_workspace_view_loading = false;
    }
    if (m_core.has_documents()) {
        m_core.set_current_document(wv.m_current_document);
        set_current_group(get_current_document_view().m_current_group);
    }
    update_action_sensitivity();
    m_workspace_browser->update_current_group(get_current_document_views());
}

void Editor::save_workspace_view(const UUID &doc_uu)
{
    auto &doci = m_core.get_idocument_info(doc_uu);
    if (!doci.has_path())
        return;
    json wsvs = WorkspaceView::serialize(m_workspace_views, doc_uu);
    if (wsvs.is_null())
        return;

    json j = {{"workspace_views", wsvs}};
    save_json_to_file(get_workspace_filename_from_document_filename(doci.get_path()), j);
}

std::filesystem::path Editor::get_workspace_filename_from_document_filename(const std::filesystem::path &path)
{
    const auto dn = path.parent_path();
    auto fn = path_to_string(path.filename());
    auto wsn = fn.substr(0, fn.size() - 3);
    return dn / path_from_string(wsn + "wrk");
}

} // namespace dune3d
