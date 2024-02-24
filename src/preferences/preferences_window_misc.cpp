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
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Enable animations",
                    "Use mass spring damper model to smooth zooming and other transitions such as rotation",
                    m_preferences, m_preferences.canvas.enable_animations);
            gr->add_row(*r);
        }
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Enable error overlay",
                    "Turn canvas red if there are redundant constraints or the sketch is overconstrained",
                    m_preferences, m_preferences.canvas.error_overlay);
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::make_managed<PreferencesGroup>("Action Bar");
        box->append(*gr);
        gr->show();
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Use action bar", "Show action bar in editors to quickly access commonly-used tools", m_preferences,
                    m_preferences.action_bar.enable);
            gr->add_row(*r);
        }
        /* {
             auto r = Gtk::manage(new PreferencesRowBool("Remember last action",
                                                         "Show last-used action in button instead of the default one",
                                                         preferences, preferences.action_bar.remember));
             gr->add_row(*r);
         }*/
        {
            auto r = Gtk::make_managed<PreferencesRowBool>("Always show", "Also show action bar if a tool is active",
                                                           m_preferences, m_preferences.action_bar.show_in_tool);
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::make_managed<PreferencesGroup>("Tool Bar");
        box->append(*gr);
        {
            auto r = Gtk::make_managed<PreferencesRowBool>(
                    "Vertical layout", "Show tool tip in a separate row rather than right to the action keys",
                    m_preferences, m_preferences.tool_bar.vertical_layout);
            gr->add_row(*r);
        }
    }
    {
        auto gr = Gtk::make_managed<PreferencesGroup>("Appearance");
        box->append(*gr);
        {
            auto r = Gtk::make_managed<PreferencesRowBool>("Dark theme", "Use dark theme variant if available",
                                                           m_preferences, m_preferences.canvas.dark_theme);
            gr->add_row(*r);
        }
    }
}

} // namespace dune3d
