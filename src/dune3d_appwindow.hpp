#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "editor_interface.hpp"
#include "action/action.hpp"
#include "preferences/preferences.hpp"
#include "document_view.hpp"
#include "dialogs/dialogs.hpp"

namespace dune3d {

class Dune3DApplication;
class Canvas;
class Preferences;
enum class ToolID;
class ActionLabelInfo;
class ToolPopover;
class WorkspaceBrowser;
class ConstraintsBox;
class GroupEditor;

class Dune3DAppWindow : public Gtk::ApplicationWindow, private EditorInterface {
public:
    static Dune3DAppWindow *create(Dune3DApplication &app);
    Dune3DAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                    class Dune3DApplication &app);

    void open_file_view(const Glib::RefPtr<Gio::File> &file);

    ~Dune3DAppWindow();

private:
    void canvas_update();
    void canvas_update_keep_selection();

    void tool_begin(ToolID id);
    void tool_process(ToolResponse &resp);
    void tool_process_one();
    void handle_cursor_move();
    double m_last_x = NAN;
    double m_last_y = NAN;
    void handle_click(unsigned int button, unsigned int n);

    glm::dvec3 get_cursor_pos() const override;
    glm::vec3 get_cam_normal() const override;
    glm::dvec3 get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const override;

    void apply_preferences();


    Gtk::Button *create_action_button(ActionToolID action);
    void attach_action_button(Gtk::Button &button, ActionToolID action);

    std::map<ActionToolID, ActionConnection> m_action_connections;

    ActionConnection &connect_action(ToolID tool_id);
    ActionConnection &connect_action(ActionToolID id, std::function<void(const ActionConnection &)> cb);
    ActionConnection &connect_action_with_source(ActionToolID id,
                                                 std::function<void(const ActionConnection &, ActionSource)> cb);

    KeySequence m_keys_current;
    KeyMatchResult keys_match(const KeySequence &keys) const;
    bool handle_action_key(unsigned int keyval, Gdk::ModifierType state);
    void handle_tool_action(const ActionConnection &conn);

    bool trigger_action(ActionToolID action, ActionSource source = ActionSource::UNKNOWN);
    bool get_action_sensitive(ActionID action) const;

    void update_action_sensitivity();

    std::map<ActionID, bool> m_action_sensitivity;

    sigc::signal<void()> m_signal_action_sensitive;
    Preferences &m_preferences;
    InToolKeySequencesPreferences m_in_tool_key_sequeces_preferences;


    void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels) override;
    void tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s);
    void tool_bar_clear_actions();
    std::vector<ActionLabelInfo> m_in_tool_action_label_infos;

    void tool_bar_set_visible(bool v);
    void tool_bar_set_tool_name(const std::string &s);
    void tool_bar_set_tool_tip(const std::string &s) override;
    void tool_bar_flash(const std::string &s) override;
    void tool_bar_flash_replace(const std::string &s) override;
    void tool_bar_append_action(Gtk::Widget &w);
    std::vector<Gtk::Widget *> m_action_widgets;
    void tool_bar_set_vertical(bool vert);

    void tool_update_data(std::unique_ptr<ToolData> data) override;
    void enable_hover_selection() override;
    std::optional<SelectableRef> get_hover_selection() const override;
    void set_no_canvas_update(bool v) override
    {
        m_no_canvas_update = v;
    }
    bool m_no_canvas_update = false;
    void canvas_update_from_tool() override;

    void set_solid_model_edge_select_mode(bool v) override
    {
        m_solid_model_edge_select_mode = v;
    }

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

    bool m_solid_model_edge_select_mode = false;

    ToolPopover *m_tool_popover = nullptr;

    Gtk::Paned *m_left_bar = nullptr;
    WorkspaceBrowser *m_workspace_browser = nullptr;


    Core m_core;
    Canvas *m_canvas = nullptr;
    Gtk::Label *m_key_hint_label = nullptr;
    Gtk::Label *m_workplane_label = nullptr;
    void update_workplane_label();

    Gtk::Notebook *m_properties_notebook = nullptr;
    ConstraintsBox *m_constraints_box = nullptr;
    Gtk::Box *m_group_editor_box = nullptr;
    GroupEditor *m_group_editor = nullptr;
    void update_group_editor();

    Dialogs m_dialogs;
    Dialogs &get_dialogs() override
    {
        return m_dialogs;
    }

    DocumentView m_document_view;

    void handle_tool_change();

    void reset_key_hint_label();

    void show_save_dialog(std::function<void()> save_cb, std::function<void()> no_save_cb);
    std::function<void()> m_after_save_cb;
    void close_document(const UUID &uu, std::function<void()> save_cb, std::function<void()> no_save_cb);

    std::optional<ActionToolID> get_doubleclick_action(const SelectableRef &sr);
    ToolID get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel);
    ToolID m_drag_tool;
    std::set<SelectableRef> m_selection_for_drag;
    glm::dvec2 m_cursor_pos_win_drag_begin;

    Gtk::InfoBar *m_version_info_bar = nullptr;
    Gtk::Label *m_version_info_bar_label = nullptr;
    void set_version_info(const std::string &s);
    void update_version_info();
};
} // namespace dune3d
