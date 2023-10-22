#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"

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

    class PreferencesWindow *show_preferences_window(guint32 timestamp = 0);


protected:
    // Override default signal handlers:
    void on_activate() override;
    void on_open(const Gio::Application::type_vec_files &files, const Glib::ustring &hint) override;
    void on_startup() override;

private:
    Dune3DAppWindow *create_appwindow();

    Preferences m_preferences;

    class PreferencesWindow *m_preferences_window = nullptr;

    void on_action_about();
};
} // namespace dune3d
