#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"
#include "logger/log_dispatcher.hpp"
#include <filesystem>

namespace dune3d {

class Dune3DAppWindow;

class Dune3DApplication : public Gtk::Application {
protected:
    Dune3DApplication();

public:
    static Glib::RefPtr<Dune3DApplication> create();

    Preferences &get_preferences()
    {
        return m_preferences;
    }

    void add_recent_item(const std::filesystem::path &path);
    class UserConfig {
    private:
        friend Dune3DApplication;
        UserConfig() = default;
        void load(const std::filesystem::path &filename);
        void save(const std::filesystem::path &filename);

    public:
        std::map<std::filesystem::path, Glib::DateTime> recent_items;
    };

    UserConfig m_user_config;

    typedef sigc::signal<void()> type_signal_recent_items_changed;
    type_signal_recent_items_changed signal_recent_items_changed()
    {
        return m_signal_recent_items_changed;
    }

    class PreferencesWindow *show_preferences_window(guint32 timestamp = 0);
    class LogWindow *show_log_window(guint32 timestamp = 0);


protected:
    // Override default signal handlers:
    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;
    void on_startup() override;
    void on_shutdown() override;

private:
    Dune3DAppWindow *create_appwindow();

    Preferences m_preferences;

    class PreferencesWindow *m_preferences_window = nullptr;

    LogDispatcher m_log_dispatcher;
    class LogWindow *m_log_window = nullptr;

    static std::filesystem::path get_user_config_filename();

    type_signal_recent_items_changed m_signal_recent_items_changed;

    void on_action_about();
};
} // namespace dune3d
