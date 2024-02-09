#include "capture_dialog.hpp"
#include "util/key_util.hpp"
#include <iostream>

namespace dune3d {

CaptureDialog::CaptureDialog(Gtk::Window *parent)
{
    set_transient_for(*parent);
    auto hb = Gtk::make_managed<Gtk::HeaderBar>();
    hb->set_show_title_buttons(false);
    set_titlebar(*hb);
    set_title("Capture key sequence");
    set_modal(true);

    auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);

    auto cancel_button = Gtk::make_managed<Gtk::Button>("Cancel");
    hb->pack_start(*cancel_button);
    cancel_button->signal_clicked().connect([this] { close(); });
    sg->add_widget(*cancel_button);


    auto ok_button = Gtk::make_managed<Gtk::Button>("OK");
    ok_button->signal_clicked().connect([this] {
        m_signal_ok.emit();
        close();
    });
    ok_button->add_css_class("suggested-action");
    hb->pack_end(*ok_button);
    sg->add_widget(*ok_button);

    m_capture_label = Gtk::make_managed<Gtk::Entry>();
    m_capture_label->set_editable(false);
    m_capture_label->set_placeholder_text("type here");

    {
        auto controller = Gtk::EventControllerKey::create();
        controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
        controller->signal_key_pressed().connect(
                [this, controller](guint keyval, guint keycode, Gdk::ModifierType state) -> bool {
                    auto ev = controller->get_current_event();
                    if (ev->is_modifier())
                        return false;
                    state &= ~ev->get_consumed_modifiers();
                    remap_keys(keyval, state);
                    state &= (Gdk::ModifierType::SHIFT_MASK | Gdk::ModifierType::CONTROL_MASK
                              | Gdk::ModifierType::ALT_MASK);
                    m_keys.emplace_back(keyval, state);
                    update();
                    return true;
                },
                false);


        m_capture_label->add_controller(controller);
    }

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    box->set_margin(10);
    box->append(*m_capture_label);

    auto reset_button = Gtk::make_managed<Gtk::Button>("Start over");
    box->append(*reset_button);
    reset_button->signal_clicked().connect([this] {
        m_keys.clear();
        m_capture_label->grab_focus();
        update();
    });


    set_child(*box);
}

void CaptureDialog::update()
{
    auto txt = key_sequence_to_string(m_keys);
    m_capture_label->set_text(txt);
}
} // namespace dune3d
