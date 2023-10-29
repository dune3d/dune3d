#include "preferences_window.hpp"
#include "preferences.hpp"
#include "util/gtk_util.hpp"
#include "preferences_window_keys.hpp"
#include "preferences_window_in_tool_keys.hpp"
#include "preferences_window_canvas.hpp"
#include <map>

namespace dune3d {

PreferencesWindow::PreferencesWindow(Preferences &prefs) : Gtk::Window(), m_preferences(prefs)
{
    set_default_size(600, 500);
    // set_type_hint();
    auto header = Gtk::make_managed<Gtk::HeaderBar>();
    header->set_show_title_buttons(true);
    set_title("Preferences");
    set_titlebar(*header);
    // header->show();

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    auto sidebar = Gtk::make_managed<Gtk::StackSidebar>();
    box->append(*sidebar);

    m_stack = Gtk::make_managed<Gtk::Stack>();
    sidebar->set_stack(*m_stack);
    box->append(*m_stack);

    {
        auto ed = CanvasPreferencesEditor::create(m_preferences);
        m_stack->add(*ed, "canvas", "Appearance");
        ed->unreference();
    }
    {
        auto ed = KeySequencesPreferencesEditor::create(m_preferences);
        m_stack->add(*ed, "keys", "Keys");
        ed->unreference();
    }
    {
        auto ed = InToolKeySequencesPreferencesEditor::create(m_preferences);
        m_stack->add(*ed, "in_tool_keys", "In-tool Keys");
        ed->unreference();
    }

    set_child(*box);
}

void PreferencesWindow::show_page(const std::string &pg)
{
    m_stack->set_visible_child(pg);
}

} // namespace dune3d
