#include "gtk_util.hpp"
#include <iostream>

namespace dune3d {
void install_esc_to_close(Gtk::Window &win)
{
    auto controller = Gtk::EventControllerKey::create();
    controller->signal_key_pressed().connect(
            [&win](guint keyval, guint keycode, Gdk::ModifierType state) {
                if (keyval == GDK_KEY_Escape) {
                    win.unset_focus();
                    win.close();
                    return true;
                }
                else {
                    return false;
                }
            },
            true);
    win.add_controller(controller);
}

void spinbutton_connect_activate(Gtk::SpinButton &sp, std::function<void()> cb)
{

    auto controller = Gtk::EventControllerKey::create();
    controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller->signal_key_pressed().connect(
            [&sp, cb](guint keyval, guint keycode, Gdk::ModifierType state) {
                std::cout << "kp " << keyval << std::endl;
                if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
                    const auto old_value = sp.get_value();
                    sp.update();
                    const auto new_value = sp.get_value();
                    if (old_value == new_value)
                        cb();
                    else
                        sp.select_region(0, -1);
                    return true;
                }
                else {
                    return false;
                }
            },
            false);
    sp.add_controller(controller);
}


} // namespace dune3d
