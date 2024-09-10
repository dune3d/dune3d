#include "gtk_util.hpp"

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

void spinbutton_connect_activate_immediate(Gtk::SpinButton &sp, std::function<void()> cb)
{
    {
        auto controller = Gtk::EventControllerKey::create();
        controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
        controller->signal_key_pressed().connect(
                [&sp, cb](guint keyval, guint keycode, Gdk::ModifierType state) {
                    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
                        sp.update();
                        cb();
                        return true;
                    }
                    return false;
                },
                false);
        sp.add_controller(controller);
    }
}

Gtk::Label &grid_attach_label_and_widget(Gtk::Grid &gr, const std::string &label, Gtk::Widget &w, int &top)
{
    auto la = Gtk::make_managed<Gtk::Label>(label);
    la->add_css_class("dim-label");
    la->set_xalign(1);
    la->show();
    gr.attach(*la, 0, top, 1, 1);
    gr.attach(w, 1, top, 1, 1);
    top++;
    return *la;
}

void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

RenameWindow::RenameWindow(const std::string &caption)
{
    auto hb = Gtk::make_managed<Gtk::HeaderBar>();
    hb->set_show_title_buttons(false);
    set_title(caption);
    set_titlebar(*hb);
    auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
    {
        auto cancel_button = Gtk::make_managed<Gtk::Button>("Cancel");
        cancel_button->signal_clicked().connect([this] { close(); });
        hb->pack_start(*cancel_button);
        sg->add_widget(*cancel_button);
    }
    {
        auto ok_button = Gtk::make_managed<Gtk::Button>("OK");
        ok_button->add_css_class("suggested-action");
        ok_button->signal_clicked().connect([this] { ok(); });
        hb->pack_end(*ok_button);
        sg->add_widget(*ok_button);
    }

    m_entry = Gtk::make_managed<Gtk::Entry>();
    m_entry->set_margin(10);
    m_entry->signal_activate().connect([this] { ok(); });
    set_child(*m_entry);
}

std::string RenameWindow::get_text() const
{
    return m_entry->get_text();
}

void RenameWindow::set_text(const std::string &text)
{
    m_entry->set_text(text);
}

void RenameWindow::ok()
{
    m_signal_changed.emit();
    close();
}


} // namespace dune3d
