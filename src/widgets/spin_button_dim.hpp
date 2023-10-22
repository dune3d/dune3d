#pragma once
#include <gtkmm.h>

namespace dune3d {
class SpinButtonDim : public Gtk::SpinButton {
public:
    SpinButtonDim();

protected:
    int on_input(double &new_value);
    bool on_output();
    // void on_populate_popup(Gtk::Menu *menu) override;
};
} // namespace dune3d
