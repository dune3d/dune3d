#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "action/action.hpp"

namespace dune3d {

class Preferences;

class ActionEditorBase : public Gtk::Box, public Changeable {
public:
    ActionEditorBase(const std::string &title, Preferences &prefs);

    void update();


protected:
    Preferences &m_prefs;

    Gtk::ListBox *m_lb = nullptr;

    virtual std::vector<KeySequence> *maybe_get_keys() = 0;
    virtual std::vector<KeySequence> &get_keys() = 0;
};
} // namespace dune3d
