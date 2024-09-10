#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"

namespace dune3d {
void install_esc_to_close(Gtk::Window &win);
void spinbutton_connect_activate(Gtk::SpinButton &sp, std::function<void()> cb);
void spinbutton_connect_activate_immediate(Gtk::SpinButton &sp, std::function<void()> cb);
Gtk::Label &grid_attach_label_and_widget(Gtk::Grid &gr, const std::string &label, Gtk::Widget &w, int &top);
void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before);

template <typename T>
void bind_widget(std::map<T, Gtk::ToggleButton *> &widgets, T &v, std::function<void(T)> extra_cb = nullptr)
{
    widgets[v]->set_active(true);
    for (auto &it : widgets) {
        T key = it.first;
        Gtk::ToggleButton *w = it.second;
        w->signal_toggled().connect([key, w, &v, extra_cb] {
            if (w->get_active()) {
                v = key;
                if (extra_cb)
                    extra_cb(key);
            }
        });
    }
}


class RenameWindow : public Gtk::Window, public Changeable {
public:
    RenameWindow(const std::string &caption);

    std::string get_text() const;

    void set_text(const std::string &text);

private:
    Gtk::Entry *m_entry = nullptr;

    void ok();
};

} // namespace dune3d
