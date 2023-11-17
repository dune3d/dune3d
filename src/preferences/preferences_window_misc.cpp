#include "preferences_window_misc.hpp"
#include "util/gtk_util.hpp"
#include "preferences.hpp"
#include "preferences_row.hpp"

namespace dune3d {

MiscPreferencesEditor::MiscPreferencesEditor(Preferences &prefs) : m_preferences(prefs)
{
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 20);
    box->set_halign(Gtk::Align::CENTER);
    box->show();
    box->set_margin(24);
    set_child(*box);

    {
        auto gr = Gtk::make_managed<PreferencesGroup>("Canvas");
        box->append(*gr);
        gr->show();
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Enable animations",
                    "Use mass spring damper model to smooth zooming and other transitions such as rotation",
                    m_preferences, m_preferences.canvas.enable_animations);
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::make_managed<PreferencesGroup>("Tool Bar");
        box->append(*gr);
        gr->show();
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Vertical layout", "Show tool tip in a separate row rather than right to the action keys",
                    m_preferences, m_preferences.tool_bar.vertical_layout);
            gr->add_row(*r);
        }
    }
}

} // namespace dune3d
