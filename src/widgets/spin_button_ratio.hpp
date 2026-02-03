#pragma once

#include <gtkmm/spinbutton.h>

#include "document/constraint/constraint_length_ratio.hpp"

namespace dune3d {
class SpinButtonRatio : public Gtk::SpinButton {
public:
    SpinButtonRatio();

protected:
    int on_input(double &new_value);
    bool on_output();
};
} // namespace dune3d
