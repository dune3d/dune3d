#include "editor.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "workspace_browser.hpp"
#include "nlohmann/json.hpp"
#include "util/fs_util.hpp"
#include "util/util.hpp"

namespace dune3d {

UUID Editor::create_workspace_view()
{
    auto uu = UUID::random();
    auto &wv = m_workspace_views[uu];
    wv.m_name = "Default";
    append_workspace_view_page(wv.m_name, uu);
    return uu;
}

UUID Editor::create_workspace_view_from_current()
{
    auto uu = UUID::random();
    auto &wv = m_workspace_views.emplace(uu, m_workspace_views.at(m_current_workspace_view)).first->second;
    wv.m_name += " (Copy)";
    append_workspace_view_page(wv.m_name, uu);
    return uu;
}


void Editor::append_workspace_view_page(const std::string &name, const UUID &uu)
{
    auto &la = m_win.append_workspace_view_page(name, uu);
    la.signal_close().connect([this, uu] { close_workspace_view(uu); });
    la.signal_rename().connect([this, uu] { rename_workspace_view(uu); });
}

void Editor::close_workspace_view(const UUID &uu)
{
    if (!m_workspace_views.contains(uu))
        return;
    if (m_workspace_views.size() == 1 && m_core.has_documents())
        return;
    m_workspace_views.erase(uu);
    m_win.remove_workspace_view_page(uu);
}

class RenameWindow : public Gtk::Window, public Changeable {
public:
    RenameWindow()
    {
        auto hb = Gtk::make_managed<Gtk::HeaderBar>();
        hb->set_show_title_buttons(false);
        set_title("Rename workspace view");
        set_titlebar(*hb);
        auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
        {
            auto cancel_button = Gtk::make_managed<Gtk::Button>("Cancel");
            cancel_button->signal_clicked().connect([this] { close(); });
            hb->pack_start(*cancel_button);
            sg->add_widget(*cancel_button);
        }
        {
            auto ok_button = Gtk::make_managed<Gtk::Button>("OK");
            ok_button->add_css_class("suggested-action");
            ok_button->signal_clicked().connect([this] { ok(); });
            hb->pack_end(*ok_button);
            sg->add_widget(*ok_button);
        }

        m_entry = Gtk::make_managed<Gtk::Entry>();
        m_entry->set_margin(10);
        m_entry->signal_activate().connect([this] { ok(); });
        set_child(*m_entry);
    }

    std::string get_text() const
    {
        return m_entry->get_text();
    }

    void set_text(const std::string &text)
    {
        m_entry->set_text(text);
    }

private:
    Gtk::Entry *m_entry = nullptr;

    void ok()
    {
        m_signal_changed.emit();
        close();
    }
};

void Editor::rename_workspace_view(const UUID &uu)
{
    if (!m_workspace_views.contains(uu))
        return;

    auto win = new RenameWindow();
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
    auto pages = m_win.get_workspace_notebook().get_pages();
    for (size_t i = 0; i < pages->get_n_items(); i++) {
        auto &page = dynamic_cast<Gtk::NotebookPage &>(*pages->get_object(i).get());
        auto &it = dynamic_cast<WorkspaceViewPage &>(*page.get_child());
        dynamic_cast<Dune3DAppWindow::WorkspaceTabLabel &>(*m_win.get_workspace_notebook().get_tab_label(it))
                .set_label(m_workspace_views.at(it.m_uuid).m_name);
    }
}

void Editor::set_current_workspace_view(const UUID &uu)
{
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

        ca.set_cam_distance(wv.m_cam_distance);
        ca.set_cam_quat(wv.m_cam_quat);
        ca.set_center(wv.m_center);
        set_perspective_projection(wv.m_projection == CanvasProjection::PERSP);
        update_view_hints();
        m_workspace_view_loading = false;
    }
    if (m_core.has_documents()) {
        m_core.set_current_document(wv.m_current_document);
        m_core.set_current_group(get_current_document_view().m_current_group);
        set_show_previous_construction_entities(
                get_current_document_view().m_show_construction_entities_from_previous_groups);
    }
    update_action_sensitivity();
    canvas_update_keep_selection();
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
