#include "action_editor.hpp"
#include "widgets/capture_dialog.hpp"
#include "preferences.hpp"

namespace dune3d {

class KeysBox : public Gtk::Box {
public:
    KeysBox(KeySequence &k) : Gtk::Box(Gtk::Orientation::HORIZONTAL, 5), keys(k)
    {
        set_margin(5);
    }
    KeySequence &keys;
};


ActionEditorBase::ActionEditorBase(const std::string &title, Preferences &prefs)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 5), m_prefs(prefs)
{
    set_margin(10);
    {
        auto l = Gtk::make_managed<Gtk::Label>();
        l->set_markup("<b>" + title + "</b>");
        append(*l);
    }

    m_lb = Gtk::make_managed<Gtk::ListBox>();
    m_lb->add_css_class("boxed-list");
    m_lb->set_activate_on_single_click(true);
    m_lb->set_selection_mode(Gtk::SelectionMode::NONE);


    {
        auto placeholder_label = Gtk::manage(new Gtk::Label());
        placeholder_label->set_margin(10);
        placeholder_label->add_css_class("dim-label");
        placeholder_label->set_text("no keys defined");
        m_lb->set_placeholder(*placeholder_label);
    }

    append(*m_lb);
    m_lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        if (auto kb = dynamic_cast<KeysBox *>(row->get_child())) {
            auto cap = new CaptureDialog(dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)));
            cap->present();
            cap->signal_ok().connect([this, cap, kb] {
                if (cap->m_keys.size()) {
                    kb->keys = cap->m_keys;
                    update();
                    m_signal_changed.emit();
                    m_prefs.signal_changed().emit();
                }
            });
        }
        else {
            auto cap = new CaptureDialog(dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)));
            cap->present();
            cap->signal_ok().connect([this, cap] {
                if (cap->m_keys.size()) {
                    get_keys().push_back(cap->m_keys);
                    update();
                    m_signal_changed.emit();
                    m_prefs.signal_changed().emit();
                }
            });
        }
    });
}

void ActionEditorBase::update()
{
    while (auto child = m_lb->get_first_child())
        m_lb->remove(*child);
    size_t i = 0;
    if (auto keys = maybe_get_keys()) {
        for (auto &it : *keys) {
            auto box = Gtk::make_managed<KeysBox>(it);
            auto la = Gtk::make_managed<Gtk::Label>(key_sequence_to_string(it));
            la->set_xalign(0);
            la->set_hexpand(true);
            auto delete_button = Gtk::make_managed<Gtk::Button>();
            delete_button->set_has_frame(false);
            delete_button->signal_clicked().connect([this, i, keys] {
                keys->erase(keys->begin() + i);
                update();
                m_signal_changed.emit();
                m_prefs.signal_changed().emit();
            });
            delete_button->set_image_from_icon_name("list-remove-symbolic");
            box->append(*la);
            box->append(*delete_button);
            m_lb->append(*box);
            i++;
        }
    }
    {
        auto la = Gtk::make_managed<Gtk::Label>("Add key sequence…");
        la->set_margin(10);
        m_lb->append(*la);
    }

    // s_signal_changed.emit();
}

} // namespace dune3d
