#pragma once
#include "editor_interface.hpp"
#include "core/core.hpp"
#include <gtkmm.h>
#include "action/action.hpp"
#include "preferences/preferences.hpp"
#include "workspace/workspace_view.hpp"
#include "dialogs/dialogs.hpp"
#include "util/action_label.hpp"
#include "document/group/group.hpp"
#include "selection_menu_creator.hpp"
#include "idocument_view_provider.hpp"

namespace dune3d {

class Dune3DApplication;
class Preferences;
enum class ToolID;
class ActionLabelInfo;
class ToolPopover;
class ConstraintsBox;
class GroupEditor;
class SelectionEditor;
class WorkspaceBrowser;
class Dune3DAppWindow;
class Canvas;
class ClippingPlaneWindow;
class SelectionFilterWindow;
class Buffer;
enum class SelectionMode;
enum class CommitMode;
enum class WorkspaceBrowserAddGroupMode;

class Editor : private EditorInterface, private IDocumentViewProvider {
public:
    Editor(Dune3DAppWindow &win, Preferences &prefs);

    void init();


    void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels) override;
    void tool_bar_set_tool_tip(const std::string &s) override;
    void tool_bar_flash(const std::string &s) override;
    void tool_bar_flash_replace(const std::string &s) override;
    glm::dvec3 get_cursor_pos() const override;
    glm::vec3 get_cam_normal() const override;
    glm::dvec3 get_cursor_pos_for_plane(glm::dvec3 origin, glm::dvec3 normal) const override;
    void tool_update_data(std::unique_ptr<ToolData> data) override;
    void enable_hover_selection(bool enable) override;
    std::optional<SelectableRef> get_hover_selection() const override;
    void set_no_canvas_update(bool v) override
    {
        m_no_canvas_update = v;
    }
    void canvas_update_from_tool() override;

    void set_solid_model_edge_select_mode(bool v) override
    {
        m_solid_model_edge_select_mode = v;
    }

    bool get_use_workplane() const override;

    void set_canvas_selection_mode(SelectionMode mode) override;

    void show_delete_items_popup(const ItemsToDelete &items_selected, const ItemsToDelete &items_all) override;

    Glib::RefPtr<Pango::Context> get_pango_context() override;

    void set_buffer(std::unique_ptr<const Buffer> buffer) override;
    const Buffer *get_buffer() const override;
    void set_first_update_group(const UUID &group) override;


    void open_file(const std::filesystem::path &path);
    bool has_file(const std::filesystem::path &path);

    ~Editor();

private:
    void init_workspace_browser();
    void init_properties_notebook();
    void init_header_bar();
    void init_actions();
    void init_tool_popover();
    void init_canvas();
    void init_view_options();

    void on_workspace_browser_group_selected(const UUID &uu_doc, const UUID &uu_group);
    void on_add_group(Group::Type group_type, WorkspaceBrowserAddGroupMode add_group_mode);
    void finish_add_group(Group *new_group);
    void on_delete_current_group();
    void on_move_group(Document::MoveGroup op);
    void on_workspace_browser_document_checked(const UUID &uu_doc, bool checked);
    void on_workspace_browser_group_checked(const UUID &uu_doc, const UUID &uu_group, bool checked);
    void on_workspace_browser_body_checked(const UUID &uu_doc, const UUID &uu_group, bool checked);
    void on_workspace_browser_body_solid_model_checked(const UUID &uu_doc, const UUID &uu_group, bool checked);
    void on_workspace_browser_activate_link(const std::string &link);
    void on_view_rotate(const ActionConnection &conn);
    void on_view_zoom(const ActionConnection &conn);
    void on_view_pan(const ActionConnection &conn);

    void on_workspace_browser_rename_body(const UUID &uu_doc, const UUID &uu_group);
    void on_workspace_browser_set_body_color(const UUID &uu_doc, const UUID &uu_group);
    void on_workspace_browser_reset_body_color(const UUID &uu_doc, const UUID &uu_group);

    void on_export_solid_model(const ActionConnection &conn);
    void on_export_paths(const ActionConnection &conn);
    void on_export_projection(const ActionConnection &conn);
    void on_open_document(const ActionConnection &conn);
    void on_save_as(const ActionConnection &conn);
    void on_create_group_action(const ActionConnection &conn);
    void on_move_group_action(const ActionConnection &conn);
    void on_align_to_workplane(const ActionConnection &conn);
    void on_center_to_workplane(const ActionConnection &conn);
    void on_look_here(const ActionConnection &conn);

    void on_explode_cluster(const ActionConnection &conn);
    void on_unexplode_cluster(const ActionConnection &conn);


    Canvas &get_canvas();
    const Canvas &get_canvas() const;

    Gtk::PopoverMenu *m_context_menu = nullptr;
    double m_rmb_last_x = NAN;
    double m_rmb_last_y = NAN;
    std::set<SelectableRef> m_context_menu_selection;
    enum class ContextMenuMode { ALL, CONSTRAIN };
    void open_context_menu(ContextMenuMode mode = ContextMenuMode::ALL);
    sigc::connection m_context_menu_hover_timeout;

    void set_current_group(const UUID &group);
    void canvas_update();
    void canvas_update_keep_selection();
    void render_document(const IDocumentInfo &doc);
    unsigned int m_canvas_update_pending = 0;

    class CanvasUpdater {
    public:
        [[nodiscard]] CanvasUpdater(Editor &editor);

        ~CanvasUpdater();

    private:
        Editor &m_editor;
    };

    void tool_begin(ToolID id);
    void tool_process(ToolResponse &resp);
    void tool_process_one();
    void handle_cursor_move();
    double m_last_x = NAN;
    double m_last_y = NAN;
    void handle_click(unsigned int button, unsigned int n);

    void apply_preferences();

    Gtk::Button *create_action_button(ActionToolID action);
    void attach_action_button(Gtk::Button &button, ActionToolID action);
    void attach_action_sensitive(Gtk::Widget &widget, ActionToolID action);

    Gtk::Button &create_action_bar_button(ActionToolID action);
    std::map<ActionToolID, Gtk::Button *> m_action_bar_buttons;
    void update_action_bar_buttons_sensitivity();
    void update_action_bar_visibility();
    bool force_end_tool();

    Glib::RefPtr<Gio::Menu> m_view_options_menu;
    Glib::RefPtr<Gio::SimpleAction> m_perspective_action;
    void set_perspective_projection(bool persp);
    Glib::RefPtr<Gio::SimpleAction> m_previous_construction_entities_action;
    void set_show_previous_construction_entities(bool show);
    Glib::RefPtr<Gio::SimpleAction> m_hide_irrelevant_workplanes_action;
    void set_hide_irrelevant_workplanes(bool hide);
    void add_tool_action(ActionToolID id, const std::string &action);
    Gtk::Scale *m_curvature_comb_scale = nullptr;

    void update_view_hints();

    std::map<ActionToolID, ActionConnection> m_action_connections;

    ActionConnection &connect_action(ToolID tool_id);
    ActionConnection &connect_action(ActionToolID id, std::function<void(const ActionConnection &)> cb);
    ActionConnection &connect_action_with_source(ActionToolID id,
                                                 std::function<void(const ActionConnection &, ActionSource)> cb);

    KeySequence m_keys_current;
    KeyMatchResult keys_match(const KeySequence &keys) const;
    bool handle_action_key(Glib::RefPtr<Gtk::EventControllerKey> controller, unsigned int keyval,
                           Gdk::ModifierType state);
    void handle_tool_action(const ActionConnection &conn);

    bool trigger_action(ActionToolID action, ActionSource source = ActionSource::UNKNOWN);
    bool get_action_sensitive(ActionID action) const;

    void update_action_sensitivity();
    void update_action_sensitivity(const std::set<SelectableRef> &sel);

    std::map<ActionID, bool> m_action_sensitivity;

    void tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s);
    void tool_bar_clear_actions();
    std::vector<ActionLabelInfo> m_in_tool_action_label_infos;

    sigc::signal<void()> m_signal_action_sensitive;
    Preferences &m_preferences;
    InToolKeySequencesPreferences m_in_tool_key_sequeces_preferences;


    bool m_no_canvas_update = false;
    bool m_solid_model_edge_select_mode = false;

    ToolPopover *m_tool_popover = nullptr;

    WorkspaceBrowser *m_workspace_browser = nullptr;

    void update_workplane_label();
    void update_selection_mode_label();

    Gtk::Notebook *m_properties_notebook = nullptr;
    ConstraintsBox *m_constraints_box = nullptr;
    Gtk::Box *m_group_editor_box = nullptr;
    Gtk::Revealer *m_group_commit_pending_revealer = nullptr;
    GroupEditor *m_group_editor = nullptr;
    void update_group_editor();
    sigc::connection m_delayed_commit_connection;
    void commit_from_editor();
    void handle_commit_from_editor(CommitMode mode);


    SelectionEditor *m_selection_editor = nullptr;
    void update_selection_editor();
    Gtk::Revealer *m_selection_commit_pending_revealer = nullptr;
    sigc::connection m_delayed_selection_commit_connection;
    void commit_from_selection_editor();

    Dialogs m_dialogs;
    Dialogs &get_dialogs() override
    {
        return m_dialogs;
    }

    void set_constraint_icons(glm::vec3 p, glm::vec3 v, const std::vector<ConstraintType> &constraints) override;

    std::map<UUID, WorkspaceView> m_workspace_views;
    UUID m_current_workspace_view;
    WorkspaceView &get_current_workspace_view();
    std::map<UUID, DocumentView> &get_current_document_views();

    UUID create_workspace_view();
    UUID create_workspace_view_from_current();
    void set_current_workspace_view(const UUID &uu);
    void update_workspace_view_names();
    void update_can_close_workspace_view_pages();

    bool m_workspace_view_loading = false;
    void save_workspace_view(const UUID &doc_uu);
    void append_workspace_view_page(const std::string &name, const UUID &uu);
    void close_workspace_view(const UUID &uu);
    void auto_close_workspace_views();
    void rename_workspace_view(const UUID &uu);
    UUID duplicate_workspace_view(const UUID &uu);
    static std::filesystem::path get_workspace_filename_from_document_filename(const std::filesystem::path &path);

    void load_linked_documents(const UUID &uu_doc);

    DocumentView &get_current_document_view() override;

    void handle_tool_change();

    void reset_key_hint_label();

    void show_save_dialog(const std::string &doc_name, std::function<void()> save_cb, std::function<void()> no_save_cb);
    std::function<void()> m_after_save_cb;
    void close_document(const UUID &uu, std::function<void()> save_cb, std::function<void()> no_save_cb);
    void do_close_document(const UUID &uu);

    std::optional<ActionToolID> get_doubleclick_action(const SelectableRef &sr);
    ToolID get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel);
    ToolID m_drag_tool;
    std::set<SelectableRef> m_selection_for_drag;
    glm::dvec2 m_cursor_pos_win_drag_begin;

    std::unique_ptr<ClippingPlaneWindow> m_clipping_plane_window;

    void update_version_info();

    Dune3DAppWindow &m_win;

    Core m_core;
    SelectionMode m_last_selection_mode;

    std::vector<ConstraintType> m_constraint_tip_icons;
    glm::vec3 m_constraint_tip_pos;
    glm::vec3 m_constraint_tip_vec;

    std::unique_ptr<SelectionFilterWindow> m_selection_filter_window;

    SelectionMenuCreator m_selection_menu_creator;

    void update_error_overlay();

    void update_title();
    UUID m_update_groups_after;
};
} // namespace dune3d
