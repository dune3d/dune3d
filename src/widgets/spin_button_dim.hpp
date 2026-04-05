#pragma once
#include <gtkmm.h>
namespace dune3d {
class SpinButtonDim : public Gtk::SpinButton {
public:
    SpinButtonDim();
    void set_use_inches(bool use_inches) { m_use_inches = use_inches; }
protected:
    int on_input(double &new_value);
    bool on_output();
    bool m_use_inches = false;
};
} // namespace dune3d
