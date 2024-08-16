#pragma once
#include <gtkmm.h>
#include "editor/editor.hpp"

namespace dune3d {

class Dune3DApplication;
class Canvas;

class WorkspaceViewPage : public Gtk::Widget {
public:
    WorkspaceViewPage(const UUID &uu) : m_uuid(uu)
    {
    }

    const UUID m_uuid;
};

class Dune3DAppWindow : public Gtk::ApplicationWindow {
public:
    static Dune3DAppWindow *create(Dune3DApplication &app);
    Dune3DAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                    class Dune3DApplication &app);

    void open_file_view(const Glib::RefPtr<Gio::File> &file);
    bool has_file(const std::filesystem::path &path);

    Dune3DApplication &get_app()
    {
        return m_app;
    }

    ~Dune3DAppWindow();

    Gtk::Paned &get_left_bar()
    {
        return *m_left_bar;
    }

    Gtk::Button &get_undo_button()
    {
        return *m_undo_button;
    }

    Gtk::Button &get_redo_button()
    {
        return *m_redo_button;
    }

    Gtk::Button &get_save_button()
    {
        return *m_save_button;
    }

    Gtk::Button &get_save_as_button()
    {
        return *m_save_as_button;
    }

    Gtk::Button &get_open_button()
    {
        return *m_open_button;
    }

    Gtk::Button &get_welcome_open_button()
    {
        return *m_welcome_open_button;
    }

    Gtk::MenuButton &get_open_menu_button()
    {
        return *m_open_menu_button;
    }

    Gtk::Button &get_new_button()
    {
        return *m_new_button;
    }

    Gtk::Button &get_welcome_new_button()
    {
        return *m_welcome_new_button;
    }

    Gtk::HeaderBar &get_header_bar()
    {
        return *m_header_bar;
    }

    Gtk::MenuButton &get_hamburger_menu_button()
    {
        return *m_hamburger_menu_button;
    }

    Gtk::CheckButton &get_workplane_checkbutton()
    {
        return *m_workplane_checkbutton;
    }

    Gtk::MenuButton &get_view_options_button()
    {
        return *m_view_options_button;
    }

    Canvas &get_canvas()
    {
        return *m_canvas;
    }

    const Canvas &get_canvas() const
    {
        return *m_canvas;
    }

    Gtk::Notebook &get_workspace_notebook()
    {
        return *m_workspace_notebook;
    }

    Gtk::Button &get_workspace_add_button()
    {
        return *m_workspace_add_button;
    }

    void set_key_hint_label_text(const std::string &s);

    void tool_bar_clear_actions();
    void tool_bar_append_action(Gtk::Widget &w);
    void tool_bar_set_visible(bool v);
    void tool_bar_set_tool_name(const std::string &s);
    void tool_bar_set_tool_tip(const std::string &s);
    void tool_bar_flash(const std::string &s);
    void tool_bar_flash_replace(const std::string &s);
    void tool_bar_set_vertical(bool vert);

    void set_version_info(const std::string &s);
    void set_selection_mode_label_text(const std::string &s);

    void set_welcome_box_visible(bool v);

    void add_action_button(Gtk::Widget &widget);
    void set_action_bar_visible(bool v);

    void set_view_hints_label(const std::vector<std::string> &s);
    void set_workplane_label(const std::string &s);

    void show_delete_items_popup(const std::string &expander_label, const std::string &summary_label,
                                 const std::string &detail_label);
    void hide_delete_items_popup();

    using type_signal_undo = sigc::signal<void()>;
    type_signal_undo signal_undo()
    {
        return m_signal_undo;
    }

    class WorkspaceTabLabel : public Gtk::Box {
    public:
        WorkspaceTabLabel(const std::string &label);
        void set_label(const std::string &label);
        void set_can_close(bool can_close);

        typedef sigc::signal<void()> type_signal_close;
        type_signal_close signal_close()
        {
            return m_signal_close;
        }

        type_signal_close signal_rename()
        {
            return m_signal_rename;
        }

        type_signal_close signal_duplicate()
        {
            return m_signal_duplicate;
        }


    private:
        Gtk::Label *m_label = nullptr;
        Gtk::Button *m_close_button = nullptr;
        bool m_can_close = true;
        Glib::RefPtr<Gio::SimpleAction> m_close_action;

        type_signal_close m_signal_close;
        type_signal_close m_signal_rename;
        type_signal_close m_signal_duplicate;
    };

    WorkspaceTabLabel &append_workspace_view_page(const std::string &name, const UUID &uu);
    void remove_workspace_view_page(const UUID &uu);

    void set_window_title_from_path(const std::filesystem::path &path);
    void set_window_title(const std::string &extra);

private:
    Dune3DApplication &m_app;
    Editor m_editor;

    Gtk::Button *m_open_button = nullptr;
    Gtk::MenuButton *m_open_menu_button = nullptr;
    Gtk::Button *m_new_button = nullptr;
    Gtk::Button *m_undo_button = nullptr;
    Gtk::Button *m_redo_button = nullptr;
    Gtk::Button *m_save_button = nullptr;
    Gtk::Button *m_save_as_button = nullptr;
    Gtk::SearchEntry *m_open_recent_search_entry = nullptr;

    Gtk::HeaderBar *m_header_bar = nullptr;
    Gtk::Label *m_title_label = nullptr;
    Gtk::Label *m_subtitle_label = nullptr;
    Gtk::Popover *m_open_popover = nullptr;
    Gtk::MenuButton *m_hamburger_menu_button = nullptr;
    void set_subtitle(const std::string &label);

    Gtk::ListBox *m_open_recent_listbox = nullptr;

    std::vector<Gtk::Widget *> m_action_widgets;

    Gtk::Revealer *m_tool_bar = nullptr;
    Gtk::Label *m_tool_bar_name_label = nullptr;
    Gtk::Label *m_tool_bar_tip_label = nullptr;
    Gtk::Label *m_tool_bar_flash_label = nullptr;
    Gtk::Stack *m_tool_bar_stack = nullptr;
    Gtk::Box *m_tool_bar_box = nullptr;
    Gtk::Box *m_tool_bar_actions_box = nullptr;
    sigc::connection m_tip_timeout_connection;
    std::string m_flash_text;
    bool m_tool_bar_queue_close = false;
    void tool_bar_flash(const std::string &s, bool replace);

    Gtk::Box *m_welcome_box = nullptr;
    Gtk::SearchEntry *m_welcome_recent_search_entry = nullptr;
    Gtk::ListBox *m_welcome_recent_listbox = nullptr;
    Gtk::Button *m_welcome_new_button = nullptr;
    Gtk::Button *m_welcome_open_button = nullptr;

    Gtk::Box *m_action_bar_box = nullptr;
    Gtk::Revealer *m_action_bar_revealer = nullptr;

    Gtk::Paned *m_left_bar = nullptr;

    Canvas *m_canvas = nullptr;
    Gtk::Label *m_key_hint_label = nullptr;
    Gtk::CheckButton *m_workplane_checkbutton = nullptr;
    Gtk::Label *m_workplane_label = nullptr;
    Gtk::Notebook *m_workspace_notebook = nullptr;
    Gtk::Button *m_workspace_add_button = nullptr;

    Gtk::MenuButton *m_view_options_button = nullptr;
    Gtk::Label *m_view_hints_label = nullptr;


    Gtk::InfoBar *m_version_info_bar = nullptr;
    Gtk::Label *m_version_info_bar_label = nullptr;

    Gtk::Label *m_selection_mode_label = nullptr;

    Gtk::Revealer *m_delete_revealer = nullptr;
    Gtk::Expander *m_delete_expander = nullptr;
    Gtk::Label *m_delete_detail_label = nullptr;
    Gtk::Label *m_delete_close_label = nullptr;
    Gtk::Button *m_delete_close_button = nullptr;
    Gtk::Button *m_delete_undo_button = nullptr;
    glm::vec2 m_delete_motion = {NAN, NAN};

    sigc::connection m_delete_timeout_connection;
    unsigned int m_delete_countdown = 0;
    void update_delete_close_button_label();
    void update_delete_detail_label();

    std::string m_delete_summary;
    std::string m_delete_detail;

    type_signal_undo m_signal_undo;
};
} // namespace dune3d
