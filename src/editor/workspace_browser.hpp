#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "document/group/group.hpp"
#include "document/document.hpp"

namespace dune3d {

class Core;
class DocumentView;
class IDocumentInfo;

class WorkspaceBrowser : public Gtk::Box {
public:
    WorkspaceBrowser(Core &core);

    void update_documents(const std::map<UUID, DocumentView> &doc_views);
    void update_current_group(const std::map<UUID, DocumentView> &doc_views);
    void update_needs_save();


    using type_signal_group_selected = sigc::signal<void(UUID, UUID)>;
    type_signal_group_selected signal_group_selected()
    {
        return m_signal_group_selected;
    }

    using type_signal_group_checked = sigc::signal<void(UUID, UUID, bool)>;
    using type_signal_document_checked = sigc::signal<void(UUID, bool)>;
    type_signal_group_checked signal_group_checked()
    {
        return m_signal_group_checked;
    }

    type_signal_group_checked signal_body_checked()
    {
        return m_signal_body_checked;
    }

    type_signal_group_checked signal_body_solid_model_checked()
    {
        return m_signal_body_solid_model_checked;
    }

    type_signal_document_checked signal_document_checked()
    {
        return m_signal_document_checked;
    }

    using type_signal_delete_current_group = sigc::signal<void()>;
    using type_signal_add_group = sigc::signal<void(Group::Type)>;

    type_signal_delete_current_group signal_delete_current_group()
    {
        return m_signal_delete_current_group;
    }

    type_signal_add_group signal_add_group()
    {
        return m_signal_add_group;
    }

    using type_signal_move_group = sigc::signal<void(Document::MoveGroup)>;
    type_signal_move_group signal_move_group()
    {
        return m_signal_move_group;
    }

    using type_signal_close_document = sigc::signal<void(UUID)>;
    type_signal_close_document signal_close_document()
    {
        return m_signal_close_document;
    }

    using type_signal_activate_link = sigc::signal<void(std::string)>;
    type_signal_activate_link signale_active_link()
    {
        return m_signal_activate_link;
    }


    void group_prev_next(int dir);
    void select_group(const UUID &uu);

    void show_toast(const std::string &msg);

private:
    Gtk::ListView *m_view = nullptr;


    class DocumentItem;
    class BodyItem;
    class GroupItem;
    class WorkspaceRow;
    friend class WorkspaceRow;
    Glib::RefPtr<Gio::ListStore<DocumentItem>> m_document_store;

    Glib::RefPtr<Gio::ListModel> create_model(const Glib::RefPtr<Glib::ObjectBase> &item = {});
    Glib::RefPtr<Gtk::TreeListModel> m_model;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
    Gtk::Revealer *m_toast_revealer = nullptr;
    Gtk::Label *m_toast_label = nullptr;
    Gtk::InfoBar *m_info_bar = nullptr;
    Gtk::Image *m_info_bar_icon = nullptr;
    Gtk::Label *m_info_bar_label = nullptr;

    Core &m_core;

    type_signal_group_selected m_signal_group_selected;
    type_signal_group_checked m_signal_group_checked;
    type_signal_group_checked m_signal_body_checked;
    type_signal_group_checked m_signal_body_solid_model_checked;
    type_signal_document_checked m_signal_document_checked;

    type_signal_delete_current_group m_signal_delete_current_group;
    type_signal_add_group m_signal_add_group;
    type_signal_move_group m_signal_move_group;
    type_signal_close_document m_signal_close_document;
    type_signal_activate_link m_signal_activate_link;

    void block_signals();
    void unblock_signals();

    static void update_name(DocumentItem &it, IDocumentInfo &doci);

    sigc::connection m_toast_connection;
};
} // namespace dune3d
