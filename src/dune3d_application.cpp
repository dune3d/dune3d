#include "dune3d_application.hpp"
#include "dune3d_appwindow.hpp"
#include "util/util.hpp"
#include "util/fs_util.hpp"
#include "nlohmann/json.hpp"
#include "preferences/preferences_window.hpp"
#include "widgets/about_dialog.hpp"
#include "logger/logger.hpp"
#include "widgets/log_window.hpp"
#include "widgets/log_view.hpp"
#include "logger/log_util.hpp"
#include <iostream>
#include <iomanip>

namespace dune3d {

Dune3DApplication::Dune3DApplication() : Gtk::Application("org.dune3d.dune3d", Gio::Application::Flags::HANDLES_OPEN)
{
}

Glib::RefPtr<Dune3DApplication> Dune3DApplication::create()
{
    return Glib::make_refptr_for_instance<Dune3DApplication>(new Dune3DApplication());
}

Dune3DAppWindow *Dune3DApplication::create_appwindow()
{
    auto appwindow = Dune3DAppWindow::create(*this);

    // Make sure that the application runs for as long this window is still open.
    add_window(*appwindow);

    // A window can be added to an application with Gtk::Application::add_window()
    // or Gtk::Window::set_application(). When all added windows have been hidden
    // or removed, the application stops running (Gtk::Application::run() returns()),
    // unless Gio::Application::hold() has been called.

    // Delete the window when it is hidden.
    appwindow->signal_hide().connect([appwindow]() { delete appwindow; });

    return appwindow;
}

void Dune3DApplication::on_activate()
{
    // The application has been started, so let's show a window.
    auto appwindow = create_appwindow();
    appwindow->present();
}

void Dune3DApplication::on_open(const Gio::Application::type_vec_files &files, const Glib::ustring & /* hint */)
{
    // The application has been asked to open some files,
    // so let's open a new view for each one.
    Dune3DAppWindow *appwindow = nullptr;
    auto windows = get_windows();
    if (windows.size() > 0)
        appwindow = dynamic_cast<Dune3DAppWindow *>(windows[0]);

    if (!appwindow)
        appwindow = create_appwindow();

    for (const auto &file : files)
        appwindow->open_file_view(file);

    appwindow->present();
}

const Preferences *the_preferences = nullptr;

void Dune3DApplication::on_startup()
{
    Gtk::Application::on_startup();
    create_config_dir();
    m_preferences.load_default();
    the_preferences = &m_preferences;
    try {
        m_preferences.load();
    }
    CATCH_LOG(Logger::Level::CRITICAL, "error loading preferences", Logger::Domain::UNSPECIFIED)

    // std::cout << std::setw(4) << m_preferences.serialize() << std::endl;


    add_action("preferences", [this] {
        auto pwin = show_preferences_window();
        if (auto win = get_active_window()) {
            pwin->set_transient_for(*win);
        }
    });
    add_action("logger", [this] { show_log_window(); });
    // add_action("quit", sigc::mem_fun(*this, &PoolProjectManagerApplication::on_action_quit));
    // add_action("new_window", sigc::mem_fun(*this, &PoolProjectManagerApplication::on_action_new_window));
    add_action("about", sigc::mem_fun(*this, &Dune3DApplication::on_action_about));
    // add_action("view_log", [this] { show_log_window(); });

    if (std::filesystem::is_regular_file(get_user_config_filename())) {
        m_user_config.load(get_user_config_filename());
    }

    m_log_window = new LogWindow();
    m_log_window->set_hide_on_close(true);
    m_log_dispatcher.set_handler([this](const auto &it) { m_log_window->get_view().push_log(it); });
    Logger::get().set_log_handler([this](const Logger::Item &it) { m_log_dispatcher.log(it); });
    property_active_window().signal_changed().connect([this] {
        if (auto win = get_active_window())
            m_log_window->set_transient_for(*win);
    });

    auto cssp = Gtk::CssProvider::create();
    cssp->load_from_resource("/org/dune3d/dune3d/dune3d.css");
    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), cssp,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Gtk::IconTheme::get_default()->add_resource_path("/org/horizon-eda/horizon/icons");
    Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->add_resource_path("/org/dune3d/dune3d/icons");
    Gtk::Window::set_default_icon_name("dune3d");
}

void Dune3DApplication::on_shutdown()
{
    m_user_config.save(get_user_config_filename());
    Gtk::Application::on_shutdown();
}

PreferencesWindow *Dune3DApplication::show_preferences_window(guint32 timestamp)
{
    if (!m_preferences_window) {
        m_preferences_window = new PreferencesWindow(m_preferences);
        m_preferences_window->set_hide_on_close(true);
        m_preferences_window->signal_hide().connect([this] {
            std::cout << "pref save" << std::endl;
            m_preferences.save();
            delete m_preferences_window;
            m_preferences_window = nullptr;
        });
    }
    m_preferences_window->present(timestamp);
    return m_preferences_window;
}

LogWindow *Dune3DApplication::show_log_window(guint32 timestamp)
{
    m_log_window->present(timestamp);
    return m_log_window;
}

void Dune3DApplication::on_action_about()
{
    auto dia = new AboutDialog();
    auto win = get_active_window();
    if (win)
        dia->set_transient_for(*win);
    dia->set_modal(true);
    dia->present();
}

const Preferences &Preferences::get()
{
    assert(the_preferences);
    return *the_preferences;
}

std::filesystem::path Dune3DApplication::get_user_config_filename()
{
    return get_config_dir() / "user_config.json";
}
void Dune3DApplication::UserConfig::load(const std::filesystem::path &filename)
{
    json j = load_json_from_file(filename);
    if (j.count("recent")) {
        const json &o = j["recent"];
        for (const auto &[fn, v] : o.items()) {
            auto path = path_from_string(fn);
            if (std::filesystem::is_regular_file(path))
                recent_items.emplace(path, Glib::DateTime::create_now_local(v.get<int64_t>()));
        }
    }
    if (j.count("export_paths")) {
        const json &o = j.at("export_paths");
        for (const auto &[fn, v] : o.items()) {
            for (const auto &[group, jpaths] : v.items()) {
                const auto k = std::make_pair(path_from_string(fn), UUID{group});
                auto &it = export_paths[k];
                jpaths.at("stl").get_to(it.stl);
                jpaths.at("step").get_to(it.step);
                jpaths.at("paths").get_to(it.paths);
                jpaths.at("projection").get_to(it.projection);
            }
        }
    }
}

void Dune3DApplication::UserConfig::save(const std::filesystem::path &filename)
{
    json j;
    for (const auto &[path, mod] : recent_items) {
        j["recent"][path_to_string(path)] = mod.to_unix();
    }
    for (const auto &[k, it] : export_paths) {
        j["export_paths"][path_to_string(k.first)][k.second] = {
                {"stl", it.stl},
                {"step", it.step},
                {"paths", it.paths},
                {"projection", it.projection},
        };
    }
    save_json_to_file(filename, j);
}

void Dune3DApplication::add_recent_item(const std::filesystem::path &path)
{
    m_user_config.recent_items[path] = Glib::DateTime::create_now_local();
    m_signal_recent_items_changed.emit();
}


} // namespace dune3d
