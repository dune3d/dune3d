#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
#include "tool_window.hpp"

namespace dune3d {

class EditorInterface;
class SpinButtonDim;

class ToolDataEnterDatumWindow : public ToolDataWindow {
public:
    double value = 0;
};

class EnterDatumWindow : public ToolWindow {
public:
    EnterDatumWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label, double def = 0);

    void set_range(double lo, double hi);
    void set_step_size(double sz);
    double get_value();

private:
    SpinButtonDim *m_sp = nullptr;
};
} // namespace dune3d
