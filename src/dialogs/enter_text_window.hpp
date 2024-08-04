#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include "util/datum_unit.hpp"

namespace dune3d {

class EditorInterface;

class ToolDataEnterTextWindow : public ToolDataWindow {
public:
    std::string text;
};

class EnterTextWindow : public ToolWindow {
public:
    EnterTextWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label, const std::string &def = 0);

    std::string get_text();

private:
    Gtk::Entry *m_entry = nullptr;
};
} // namespace dune3d
