#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"
#include "util/datum_unit.hpp"

namespace dune3d {

class EditorInterface;

class ToolDataEnterDatumWindow : public ToolDataWindow {
public:
    double value = 0;
};

class EnterDatumWindow : public ToolWindow {
public:
    EnterDatumWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label, DatumUnit unit,
                     double def = 0);

    void set_range(double lo, double hi);
    void set_step_size(double sz);
    double get_value();

private:
    Gtk::SpinButton *m_sp = nullptr;
};
} // namespace dune3d
