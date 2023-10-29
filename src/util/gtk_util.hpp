#pragma once
#include <gtkmm.h>

namespace dune3d {
void install_esc_to_close(Gtk::Window &win);
void spinbutton_connect_activate(Gtk::SpinButton &sp, std::function<void()> cb);
void spinbutton_connect_activate_immediate(Gtk::SpinButton &sp, std::function<void()> cb);
Gtk::Label &grid_attach_label_and_widget(Gtk::Grid &gr, const std::string &label, Gtk::Widget &w, int &top);
} // namespace dune3d
