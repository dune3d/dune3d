#include "dune3d_application.hpp"
#include "dune3d_appwindow.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "preferences/preferences_window.hpp"
#include "widgets/about_dialog.hpp"
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
    m_preferences.load();
    // std::cout << std::setw(4) << m_preferences.serialize() << std::endl;
    the_preferences = &m_preferences;

    add_action("preferences", [this] {
        auto pwin = show_preferences_window();
        if (auto win = get_active_window()) {
            pwin->set_transient_for(*win);
        }
    });
    // add_action("logger", [this] { show_log_window(); });
    // add_action("quit", sigc::mem_fun(*this, &PoolProjectManagerApplication::on_action_quit));
    // add_action("new_window", sigc::mem_fun(*this, &PoolProjectManagerApplication::on_action_new_window));
    add_action("about", sigc::mem_fun(*this, &Dune3DApplication::on_action_about));
    // add_action("view_log", [this] { show_log_window(); });

    auto cssp = Gtk::CssProvider::create();
    cssp->load_from_resource("/org/dune3d/dune3d/dune3d.css");
    Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), cssp,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // Gtk::IconTheme::get_default()->add_resource_path("/org/horizon-eda/horizon/icons");
    Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->add_resource_path("/org/dune3d/dune3d/icons");
    Gtk::Window::set_default_icon_name("dune3d");
}

PreferencesWindow *Dune3DApplication::show_preferences_window(guint32 timestamp)
{
    if (!m_preferences_window) {
        m_preferences_window = new PreferencesWindow(m_preferences);

        m_preferences_window->signal_destroy().connect([this] {
            std::cout << "pref save" << std::endl;
            m_preferences_window = nullptr;
            m_preferences.save();
        });
    }
    m_preferences_window->present(timestamp);
    return m_preferences_window;
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


} // namespace dune3d
