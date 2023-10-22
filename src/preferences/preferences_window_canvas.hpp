#pragma once
#include <gtkmm.h>
#include "action/action.hpp"
#include "preferences.hpp"

namespace dune3d {

class CanvasPreferencesEditor : public Gtk::Box {
public:
    CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs);
    static CanvasPreferencesEditor *create(Preferences &prefs);

private:
    class Preferences &m_preferences;
    class CanvasPreferences &m_canvas_preferences;

    Gtk::FlowBox *m_colors_box = nullptr;
    Glib::RefPtr<Gtk::ColorChooser> m_color_chooser;
    sigc::connection m_color_chooser_conn;

    std::vector<unsigned int> m_msaa_settings;

    void update_color_chooser();
};

} // namespace dune3d
