#pragma once
#include <gtkmm.h>
#include "editor/editor.hpp"

namespace dune3d {

class Dune3DApplication;
class Canvas;

class Dune3DAppWindow : public Gtk::ApplicationWindow {
public:
    static Dune3DAppWindow *create(Dune3DApplication &app);
    Dune3DAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                    class Dune3DApplication &app);

    void open_file_view(const Glib::RefPtr<Gio::File> &file);

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

    Gtk::MenuButton &get_open_menu_button()
    {
        return *m_open_menu_button;
    }

    Gtk::Button &get_new_button()
    {
        return *m_new_button;
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

    Canvas &get_canvas()
    {
        return *m_canvas;
    }

    const Canvas &get_canvas() const
    {
        return *m_canvas;
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
    Gtk::Popover *m_open_popover = nullptr;
    Gtk::MenuButton *m_hamburger_menu_button = nullptr;

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


    Gtk::Paned *m_left_bar = nullptr;

    Canvas *m_canvas = nullptr;
    Gtk::Label *m_key_hint_label = nullptr;
    Gtk::CheckButton *m_workplane_checkbutton = nullptr;


    Gtk::InfoBar *m_version_info_bar = nullptr;
    Gtk::Label *m_version_info_bar_label = nullptr;

    Gtk::Label *m_selection_mode_label = nullptr;
};
} // namespace dune3d
