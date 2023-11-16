#pragma once
#include <gtkmm.h>

namespace dune3d {
class MiscPreferencesEditor : public Gtk::ScrolledWindow {
public:
    MiscPreferencesEditor(class Preferences &prefs);

private:
    Preferences &m_preferences;
};

} // namespace dune3d
