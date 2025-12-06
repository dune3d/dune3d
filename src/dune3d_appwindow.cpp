#include "dune3d_application.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "widgets/axes_lollipop.hpp"
#include "widgets/recent_item_box.hpp"
#include "util/fs_util.hpp"
#include "util/gtk_util.hpp"
#include <format>

namespace dune3d {

Dune3DAppWindow *Dune3DAppWindow::create(Dune3DApplication &app)
{
    // Load the Builder file and instantiate its widgets.

    auto refBuilder = Gtk::Builder::create_from_resource("/org/dune3d/dune3d/window.ui");

    auto window = Gtk::Builder::get_widget_derived<Dune3DAppWindow>(refBuilder, "window", app);

    if (!window)
        throw std::runtime_error("No \"window\" object in window.ui");
    return window;
}

Dune3DAppWindow::~Dune3DAppWindow() = default;

static std::vector<std::pair<std::filesystem::path, Glib::DateTime>>
recent_sort(const std::map<std::filesystem::path, Glib::DateTime> &recent_items)
{
    std::vector<std::pair<std::filesystem::path, Glib::DateTime>> recent_items_sorted;

    recent_items_sorted.reserve(recent_items.size());
    for (const auto &it : recent_items) {
        recent_items_sorted.emplace_back(it.first, it.second);
    }
    std::sort(recent_items_sorted.begin(), recent_items_sorted.end(),
              [](const auto &a, const auto &b) { return a.second.to_unix() > b.second.to_unix(); });
    return recent_items_sorted;
}

static void update_recent_listbox(Gtk::ListBox &lb, Dune3DApplication &app)
{
    while (auto child = lb.get_first_child())
        lb.remove(*child);

    const auto recent_items_sorted = recent_sort(app.m_user_config.recent_items);

    for (const auto &[path, mtime] : recent_items_sorted) {
        const auto name = path_to_string(path.filename());
        auto box = Gtk::make_managed<RecentItemBox>(name, path, mtime);
        lb.append(*box);
        box->show();
        box->signal_remove().connect([box, &app] {
            app.m_user_config.recent_items.erase(box->m_path);
            // Glib::signal_idle().connect_once([this] { update_recent_items(); });
        });
    }
}

static void update_recent_search(Gtk::SearchEntry &entry, Gtk::ListBox &lb)
{
    std::string needle = entry.get_text();
    const auto uneedle = Glib::ustring(needle).casefold();

    int i = 0;
    while (auto row = lb.get_row_at_index(i++)) {
        if (auto box = dynamic_cast<RecentItemBox *>(row->get_child())) {
            const bool visible =
                    (uneedle.size() == 0)
                    || (Glib::ustring(box->get_name_without_suffix()).casefold().find(needle) != Glib::ustring::npos);
            row->set_visible(visible);
        }
    }
}

Dune3DAppWindow::Dune3DAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                 class Dune3DApplication &app)
    : Gtk::ApplicationWindow(cobject), m_app(app), m_editor(*this, app.get_preferences())
{
    m_canvas = Gtk::make_managed<Canvas>();

    {
        auto paned = refBuilder->get_widget<Gtk::Paned>("paned");
        paned->set_shrink_start_child(false);
    }

    m_left_bar = refBuilder->get_widget<Gtk::Paned>("left_bar");


    {
        auto pick_button = refBuilder->get_widget<Gtk::Button>("pick_button");
        pick_button->signal_clicked().connect(
                [this] { get_canvas().queue_pick(m_app.get_preferences().canvas.pick_path); });
        pick_button->set_visible(!app.get_preferences().canvas.pick_path.empty());
    }

    m_header_bar = refBuilder->get_widget<Gtk::HeaderBar>("titlebar");
    m_title_label = refBuilder->get_widget<Gtk::Label>("title_label");
    m_subtitle_label = refBuilder->get_widget<Gtk::Label>("subtitle_label");

    m_open_button = refBuilder->get_widget<Gtk::Button>("open_button");
    m_open_popover = refBuilder->get_widget<Gtk::Popover>("open_popover");
    m_open_menu_button = refBuilder->get_widget<Gtk::MenuButton>("open_menu_button");
    m_new_button = refBuilder->get_widget<Gtk::Button>("new_button");
    m_save_button = refBuilder->get_widget<Gtk::Button>("save_button");
    m_save_as_button = refBuilder->get_widget<Gtk::Button>("save_as_button");
    m_open_recent_listbox = refBuilder->get_widget<Gtk::ListBox>("open_recent_listbox");
    m_open_recent_search_entry = refBuilder->get_widget<Gtk::SearchEntry>("open_recent_search_entry");
    m_open_recent_listbox->set_header_func(sigc::ptr_fun(header_func_separator));
    m_open_recent_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto &ch = dynamic_cast<RecentItemBox &>(*row->get_child());
        m_open_popover->popdown();
        m_editor.open_file(ch.m_path);
    });
    m_open_popover->signal_show().connect([this] {
        m_open_recent_search_entry->set_text("");
        update_recent_listbox(*m_open_recent_listbox, m_app);
    });
    m_open_recent_search_entry->signal_changed().connect(
            [this] { update_recent_search(*m_open_recent_search_entry, *m_open_recent_listbox); });
    m_open_recent_search_entry->signal_stop_search().connect([this] { m_open_popover->popdown(); });


    m_hamburger_menu_button = refBuilder->get_widget<Gtk::MenuButton>("hamburger_menu_button");


    m_tool_bar = refBuilder->get_widget<Gtk::Revealer>("tool_bar");
    m_tool_bar_actions_box = refBuilder->get_widget<Gtk::Box>("tool_bar_actions_box");
    m_tool_bar_box = refBuilder->get_widget<Gtk::Box>("tool_bar_box");
    m_tool_bar_name_label = refBuilder->get_widget<Gtk::Label>("tool_bar_name_label");
    m_tool_bar_tip_label = refBuilder->get_widget<Gtk::Label>("tool_bar_tip_label");
    m_tool_bar_flash_label = refBuilder->get_widget<Gtk::Label>("tool_bar_flash_label");
    m_tool_bar_stack = refBuilder->get_widget<Gtk::Stack>("tool_bar_stack");
    m_tool_bar_stack->set_visible_child(*m_tool_bar_box);


    m_workspace_notebook = refBuilder->get_widget<Gtk::Notebook>("workspace_notebook");

    m_workspace_add_button = Gtk::make_managed<Gtk::Button>();
    m_workspace_add_button->set_has_frame(false);
    m_workspace_add_button->set_icon_name("list-add-symbolic");
    m_workspace_notebook->set_action_widget(m_workspace_add_button, Gtk::PackType::END);

    refBuilder->get_widget<Gtk::Box>("canvas_box")->insert_child_at_start(*m_canvas);
    get_canvas().set_vexpand(true);
    get_canvas().set_hexpand(true);
    m_key_hint_label = refBuilder->get_widget<Gtk::Label>("key_hint_label");
    m_workplane_checkbutton = refBuilder->get_widget<Gtk::CheckButton>("workplane_checkbutton");
    m_workplane_label = refBuilder->get_widget<Gtk::Label>("workplane_label");

    {
        Gtk::Box *lollipop_box = refBuilder->get_widget<Gtk::Box>("lollipop_box");
        auto axes_lollipop = Gtk::make_managed<AxesLollipop>();
        lollipop_box->append(*axes_lollipop);
        get_canvas().signal_view_changed().connect(sigc::track_obj(
                [this, axes_lollipop] { axes_lollipop->set_quat(get_canvas().get_cam_quat()); }, *axes_lollipop));
        axes_lollipop->set_quat(get_canvas().get_cam_quat());

        // Connect to the snap signal
        axes_lollipop->signal_snap_to_plane().connect([this](int plane) {
            switch (plane) {
            case 0: // YZ plane (looking down X axis)
            {
                glm::quat q = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0));
                get_canvas().animate_to_cam_quat(q);
            } break;
            case 1: // XZ plane (looking down Y axis)
            {
                glm::quat q = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
                get_canvas().animate_to_cam_quat(q);
            } break;
            case 2: // XY plane (looking down Z axis)
            {
                glm::quat q = glm::angleAxis(0.0f, glm::vec3(1, 0, 0));
                get_canvas().animate_to_cam_quat(q);
            } break;
            }
        });
    }


    m_version_info_bar = refBuilder->get_widget<Gtk::InfoBar>("version_info_bar");
    m_version_info_bar_label = refBuilder->get_widget<Gtk::Label>("version_info_bar_label");

    m_selection_mode_label = refBuilder->get_widget<Gtk::Label>("selection_mode_label");

    m_welcome_box = refBuilder->get_widget<Gtk::Box>("welcome_box");
    m_welcome_recent_listbox = refBuilder->get_widget<Gtk::ListBox>("welcome_recent_listbox");
    m_welcome_recent_listbox->set_header_func(sigc::ptr_fun(header_func_separator));
    m_welcome_recent_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto &ch = dynamic_cast<RecentItemBox &>(*row->get_child());
        m_editor.open_file(ch.m_path);
    });
    m_welcome_recent_search_entry = refBuilder->get_widget<Gtk::SearchEntry>("welcome_recent_search_entry");
    m_welcome_new_button = refBuilder->get_widget<Gtk::Button>("welcome_new_button");
    m_welcome_open_button = refBuilder->get_widget<Gtk::Button>("welcome_open_button");

    {
        auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::VERTICAL);
        sg->add_widget(*m_welcome_new_button);
        sg->add_widget(*refBuilder->get_widget<Gtk::Label>("welcome_recent_label"));
    }

    m_action_bar_box = refBuilder->get_widget<Gtk::Box>("action_bar_box");
    m_action_bar_revealer = refBuilder->get_widget<Gtk::Revealer>("action_bar_revealer");

    m_view_options_button = refBuilder->get_widget<Gtk::MenuButton>("view_options_button");
    m_view_hints_label = refBuilder->get_widget<Gtk::Label>("view_hints_label");

    m_delete_revealer = refBuilder->get_widget<Gtk::Revealer>("delete_revealer");
    m_delete_expander = refBuilder->get_widget<Gtk::Expander>("delete_expander");
    m_delete_detail_label = refBuilder->get_widget<Gtk::Label>("delete_detail_label");
    m_delete_undo_button = refBuilder->get_widget<Gtk::Button>("delete_undo_button");
    m_delete_close_button = refBuilder->get_widget<Gtk::Button>("delete_close_button");
    m_delete_close_label = refBuilder->get_widget<Gtk::Label>("delete_close_label");
    {
        auto controller = Gtk::EventControllerMotion::create();
        controller->signal_motion().connect([this](double x, double y) {
            if (isnan(m_delete_motion.x))
                m_delete_motion = {x, y};
            if (glm::length(m_delete_motion - glm::vec2(x, y)) > 8) {
                m_delete_timeout_connection.disconnect();
                update_delete_close_button_label();
            }
        });

        m_delete_revealer->add_controller(controller);
    }
    m_delete_close_button->signal_clicked().connect(sigc::mem_fun(*this, &Dune3DAppWindow::hide_delete_items_popup));
    hide_delete_items_popup();
    m_delete_undo_button->signal_clicked().connect([this] { m_signal_undo.emit(); });
    m_delete_expander->property_expanded().signal_changed().connect(
            sigc::mem_fun(*this, &Dune3DAppWindow::update_delete_detail_label));

    set_view_hints_label({});

    update_recent_listbox(*m_welcome_recent_listbox, m_app);
    m_welcome_recent_search_entry->signal_changed().connect(
            [this] { update_recent_search(*m_welcome_recent_search_entry, *m_welcome_recent_listbox); });

    set_icon_name("dune3d");

    m_editor.init();
}

void Dune3DAppWindow::set_key_hint_label_text(const std::string &s)
{
    m_key_hint_label->set_text(s);
}

void Dune3DAppWindow::set_selection_mode_label_text(const std::string &s)
{
    m_selection_mode_label->set_text(s);
}


void Dune3DAppWindow::tool_bar_set_visible(bool v)
{
    if (v == false && m_tip_timeout_connection) { // hide and tip is still shown
        m_tool_bar_queue_close = true;
    }
    else {
        m_tool_bar->set_reveal_child(v);
        if (v) {
            m_tool_bar_queue_close = false;
        }
    }
}

void Dune3DAppWindow::tool_bar_set_tool_name(const std::string &s)
{
    m_tool_bar_name_label->set_text(s);
}

void Dune3DAppWindow::tool_bar_set_tool_tip(const std::string &s)
{
    if (s.size()) {
        m_tool_bar_tip_label->set_markup(s);
        m_tool_bar_tip_label->show();
    }
    else {
        m_tool_bar_tip_label->hide();
    }
}

void Dune3DAppWindow::tool_bar_flash(const std::string &s)
{
    tool_bar_flash(s, false);
}

void Dune3DAppWindow::tool_bar_flash_replace(const std::string &s)
{
    tool_bar_flash(s, true);
}

void Dune3DAppWindow::tool_bar_flash(const std::string &s, bool replace)
{
    if (m_flash_text.size() && !replace)
        m_flash_text += "; " + s;
    else
        m_flash_text = s;

    m_tool_bar_flash_label->set_markup(m_flash_text);

    m_tool_bar_stack->set_visible_child(*m_tool_bar_flash_label);
    m_tip_timeout_connection.disconnect();
    m_tip_timeout_connection = Glib::signal_timeout().connect(
            [this] {
                m_tool_bar_stack->set_visible_child(*m_tool_bar_box);

                m_flash_text.clear();
                if (m_tool_bar_queue_close)
                    m_tool_bar->set_reveal_child(false);
                m_tool_bar_queue_close = false;
                return false;
            },
            2000);
}

void Dune3DAppWindow::tool_bar_set_vertical(bool v)
{
    m_tool_bar_box->set_orientation(v ? Gtk::Orientation::VERTICAL : Gtk::Orientation::HORIZONTAL);
}

void Dune3DAppWindow::tool_bar_append_action(Gtk::Widget &w)
{
    m_tool_bar_actions_box->append(w);
    m_action_widgets.push_back(&w);
    m_tool_bar_actions_box->show();
}

void Dune3DAppWindow::tool_bar_clear_actions()
{
    for (auto w : m_action_widgets) {
        m_tool_bar_actions_box->remove(*w);
    }
    m_action_widgets.clear();
}


void Dune3DAppWindow::set_version_info(const std::string &s)
{
    if (s.size()) {
        m_version_info_bar->set_revealed(true);
        m_version_info_bar_label->set_markup(s);
    }
    else {
        m_version_info_bar->set_revealed(false);
    }
}

void Dune3DAppWindow::open_file_view(const Glib::RefPtr<Gio::File> &file)
{
    m_editor.open_file(path_from_string(file->get_path()));
}

bool Dune3DAppWindow::has_file(const std::filesystem::path &path)
{
    return m_editor.has_file(path);
}

void Dune3DAppWindow::set_welcome_box_visible(bool v)
{
    m_welcome_box->set_visible(v);
    if (v) {
        m_welcome_recent_search_entry->set_text("");
        update_recent_listbox(*m_welcome_recent_listbox, m_app);
    }
}

void Dune3DAppWindow::add_action_button(Gtk::Widget &widget)
{
    m_action_bar_box->append(widget);
}

void Dune3DAppWindow::set_action_bar_visible(bool v)
{
    m_action_bar_revealer->set_reveal_child(v);
}

void Dune3DAppWindow::set_view_hints_label(const std::vector<std::string> &s)
{
    if (s.size()) {
        std::string label_text;
        std::string tooltip_text;
        for (const auto &it : s) {
            if (label_text.size())
                label_text += ", ";
            if (tooltip_text.size())
                tooltip_text += "\n";
            label_text += it;
            tooltip_text += it;
        }
        m_view_hints_label->set_markup("<b>" + Glib::Markup::escape_text(label_text) + "</b>");
        m_view_hints_label->set_tooltip_text(tooltip_text);
    }
    else {
        m_view_hints_label->set_text("View");
        m_view_hints_label->set_has_tooltip(false);
    }
}

void Dune3DAppWindow::set_workplane_label(const std::string &s)
{
    m_workplane_label->set_text(s);
}

void Dune3DAppWindow::update_delete_close_button_label()
{
    std::string label = "Close";
    if (m_delete_timeout_connection.connected())
        label += std::format(" ({})", m_delete_countdown);
    m_delete_close_label->set_text(label);
}

void Dune3DAppWindow::update_delete_detail_label()
{
    if (m_delete_expander->get_expanded())
        m_delete_detail_label->set_label(m_delete_detail);
    else
        m_delete_detail_label->set_label(m_delete_summary);
}

void Dune3DAppWindow::show_delete_items_popup(const std::string &expander_label, const std::string &summary_label,
                                              const std::string &detail_label)
{
    m_delete_motion = {NAN, NAN};
    m_delete_expander->set_expanded(false);
    m_delete_expander->set_label(expander_label);
    m_delete_detail = detail_label;
    m_delete_summary = summary_label;
    update_delete_detail_label();

    m_delete_countdown = 3;
    m_delete_timeout_connection = Glib::signal_timeout().connect_seconds(
            [this] {
                if (m_delete_countdown <= 1) {
                    hide_delete_items_popup();
                    return false;
                }

                m_delete_countdown--;
                update_delete_close_button_label();

                return true;
            },
            1);
    update_delete_close_button_label();
    m_delete_revealer->set_visible(true);
}

void Dune3DAppWindow::hide_delete_items_popup()
{
    m_delete_revealer->set_visible(false);
    m_delete_timeout_connection.disconnect();
}

Dune3DAppWindow::WorkspaceTabLabel::WorkspaceTabLabel(const std::string &label) : Gtk::Box(Gtk::Orientation::HORIZONTAL)
{
    m_label = Gtk::make_managed<Gtk::Label>(label);
    append(*m_label);

    m_close_button = Gtk::make_managed<Gtk::Button>();
    m_close_button->set_image_from_icon_name("window-close-symbolic");
    m_close_button->set_has_frame(false);
    m_close_button->signal_clicked().connect([this] { m_signal_close.emit(); });
    append(*m_close_button);

    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(2);
        controller->signal_pressed().connect([this](int, double, double) {
            if (m_can_close)
                m_signal_close.emit();
        });
        add_controller(controller);
    }
    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(1);
        controller->signal_pressed().connect([this](int n_press, double, double) {
            if (n_press == 2)
                m_signal_rename.emit();
        });
        add_controller(controller);
    }

    auto popover = Gtk::make_managed<Gtk::PopoverMenu>();
    auto menu = Gio::Menu::create();
    auto actions = Gio::SimpleActionGroup::create();
    m_close_action = actions->add_action("close", [this] { m_signal_close.emit(); });
    actions->add_action("rename", [this] { m_signal_rename.emit(); });
    actions->add_action("duplicate", [this] { m_signal_duplicate.emit(); });
    insert_action_group("view", actions);
    menu->append("Duplicate", "view.duplicate");
    menu->append("Close", "view.close");
    menu->append("Rename", "view.rename");

    popover->set_menu_model(menu);
    popover->set_parent(*this);
    {
        auto controller = Gtk::GestureClick::create();
        controller->set_button(3);
        controller->signal_pressed().connect([popover](int n_press, double, double) { popover->popup(); });
        add_controller(controller);
    }
}

void Dune3DAppWindow::WorkspaceTabLabel::set_label(const std::string &label)
{
    m_label->set_label(label);
}

void Dune3DAppWindow::WorkspaceTabLabel::set_can_close(bool can_close)
{
    m_close_button->set_sensitive(can_close);
    m_can_close = can_close;
    m_close_action->set_enabled(can_close);
}

Dune3DAppWindow::WorkspaceTabLabel &Dune3DAppWindow::append_workspace_view_page(const std::string &name, const UUID &uu)
{
    auto pg = Gtk::make_managed<WorkspaceViewPage>(uu);
    auto la = Gtk::make_managed<WorkspaceTabLabel>(name);
    m_workspace_notebook->append_page(*pg, *la);
    return *la;
}

void Dune3DAppWindow::remove_workspace_view_page(const UUID &uu)
{
    auto pages = m_workspace_notebook->get_pages();
    for (size_t i = 0; i < pages->get_n_items(); i++) {
        auto &page = dynamic_cast<Gtk::NotebookPage &>(*pages->get_object(i).get());
        auto &it = dynamic_cast<WorkspaceViewPage &>(*page.get_child());
        if (it.m_uuid == uu) {
            m_workspace_notebook->remove_page(it);
            return;
        }
    }
}


void Dune3DAppWindow::set_window_title_from_path(const std::filesystem::path &path)
{
    if (path.empty()) {
        set_window_title(std::string{});
    }
    else {
        auto filename = path_to_string(path.filename());
        set_window_title(filename);
        set_subtitle(path_to_string(path.parent_path()));
    }
}

void Dune3DAppWindow::set_window_title(const std::string &extra)
{
    std::string title = "Dune 3D";
    if (!extra.empty()) {
        title = extra + " - " + title;
    }
    set_title(title);
    m_title_label->set_label(title);
    set_subtitle("");
}

void Dune3DAppWindow::set_subtitle(const std::string &label)
{
    m_subtitle_label->set_visible(label.size());
    m_subtitle_label->set_label(label);
}


} // namespace dune3d
