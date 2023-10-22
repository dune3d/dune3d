#pragma once
#include <gtkmm.h>

namespace dune3d {
void install_esc_to_close(Gtk::Window &win);
void spinbutton_connect_activate(Gtk::SpinButton &sp, std::function<void()> cb);
} // namespace dune3d
