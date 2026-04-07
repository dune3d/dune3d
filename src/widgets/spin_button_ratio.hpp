#pragma once
#include <gtkmm.h>

namespace dune3d {
class SpinButtonRatio : public Gtk::SpinButton {
public:
    SpinButtonRatio();

protected:
    int on_input(double &new_value);
    bool on_output();
};
} // namespace dune3d
