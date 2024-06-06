#pragma once
#include <gtkmm.h>

namespace dune3d {

class PreferencesWindow : public Gtk::Window {
public:
    PreferencesWindow(class Preferences &pr);
    void show_page(const std::string &pg);

private:
    class Preferences &m_preferences;
    Gtk::Stack *m_stack = nullptr;
};
} // namespace dune3d
