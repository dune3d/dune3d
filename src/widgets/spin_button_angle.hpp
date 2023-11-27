#pragma once
#include <gtkmm.h>

namespace dune3d {

class SpinButtonAngle : public Gtk::SpinButton {
public:
    SpinButtonAngle();

protected:
    int on_input(double &new_value);
    bool on_output();
    void on_wrapped();
};
} // namespace dune3d
