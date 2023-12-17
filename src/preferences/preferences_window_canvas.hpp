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
    Gtk::Revealer *m_colors_revealer = nullptr;
    Gtk::DropDown *m_theme_dropdown = nullptr;
    Gtk::ToggleButton *m_theme_variant_auto_button = nullptr;
    Gtk::ToggleButton *m_theme_variant_light_button = nullptr;
    Gtk::ToggleButton *m_theme_variant_dark_button = nullptr;
    Gtk::Button *m_theme_customize_button = nullptr;

    Gtk::FlowBox *m_colors_box = nullptr;
    std::map<ColorP, class ColorEditorPalette *> m_color_editors;
    Glib::RefPtr<Gtk::ColorChooser> m_color_chooser;
    sigc::connection m_color_chooser_conn;

    std::vector<unsigned int> m_msaa_settings;
    std::vector<std::string> m_themes;
    void update_colors_revealer();

    void update_color_chooser();

    void handle_import();
    void handle_export();
};

} // namespace dune3d
