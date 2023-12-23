#include "dune3d_application.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "widgets/axes_lollipop.hpp"
#include "widgets/recent_item_box.hpp"
#include "util/fs_util.hpp"
#include "util/gtk_util.hpp"

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


    // auto pick_button = refBuilder->get_widget<Gtk::Button>("pick_button");
    // pick_button->signal_clicked().connect([this] { get_canvas().queue_pick(); });

    m_header_bar = refBuilder->get_widget<Gtk::HeaderBar>("titlebar");

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

} // namespace dune3d
