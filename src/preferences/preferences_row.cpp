#include "preferences_row.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

static void make_label_small(Gtk::Label *la)
{
    auto attributes_list = pango_attr_list_new();
    auto attribute_scale = pango_attr_scale_new(.833);
    pango_attr_list_insert(attributes_list, attribute_scale);
    gtk_label_set_attributes(la->gobj(), attributes_list);
    pango_attr_list_unref(attributes_list);
}

static void bind_widget(Gtk::Switch *sw, bool &v)
{
    sw->set_active(v);
    sw->property_active().signal_changed().connect([sw, &v] { v = sw->get_active(); });
}


static Gtk::Box *make_boolean_ganged_switch(bool &v, const std::string &label_false, const std::string &label_true,
                                            std::function<void(bool v)> extra_cb)
{
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    box->set_homogeneous(true);
    box->get_style_context()->add_class("linked");
    auto b1 = Gtk::make_managed<Gtk::ToggleButton>(label_false);
    box->append(*b1);

    auto b2 = Gtk::make_managed<Gtk::ToggleButton>(label_true);
    b2->set_group(*b1);
    box->append(*b2);

    b2->set_active(v);
    b2->signal_toggled().connect([b2, &v, extra_cb] {
        v = b2->get_active();
        if (extra_cb)
            extra_cb(v);
    });
    return box;
}

PreferencesRow::PreferencesRow(const std::string &title, const std::string &subtitle, Preferences &prefs)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 48), preferences(prefs)
{
    set_valign(Gtk::Align::CENTER);
    set_margin(10);
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    box->set_hexpand(true);

    {
        auto la = Gtk::make_managed<Gtk::Label>();
        la->set_xalign(0);
        la->set_text(title);
        box->append(*la);
        label_title = la;
    }
    if (subtitle != "NONE") {
        auto la = Gtk::make_managed<Gtk::Label>();
        la->set_xalign(0);
        la->set_text(subtitle);
        la->get_style_context()->add_class("dim-label");
        make_label_small(la);
        box->append(*la);
        label_subtitle = la;
    }

    append(*box);
}

void PreferencesRow::set_title(const std::string &t)
{
    label_title->set_text(t);
}

void PreferencesRow::set_subtitle(const std::string &t)
{
    if (label_subtitle)
        label_subtitle->set_text(t);
}

PreferencesRowBool::PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs,
                                       bool &v)
    : PreferencesRow(title, subtitle, prefs)
{
    sw = Gtk::manage(new Gtk::Switch);
    sw->set_valign(Gtk::Align::CENTER);
    sw->show();
    append(*sw);
    bind_widget(sw, v);
    sw->property_active().signal_changed().connect([this] { preferences.signal_changed().emit(); });
}

void PreferencesRowBool::activate()
{
    sw->set_active(!sw->get_active());
}

PreferencesRowBoolButton::PreferencesRowBoolButton(const std::string &title, const std::string &subtitle,
                                                   const std::string &label_true, const std::string &label_false,
                                                   Preferences &prefs, bool &v)
    : PreferencesRow(title, subtitle, prefs)
{
    auto box = make_boolean_ganged_switch(v, label_false, label_true,
                                          [this](bool x) { preferences.signal_changed().emit(); });
    append(*box);
}


PreferencesGroup::PreferencesGroup(const std::string &title) : Gtk::Box(Gtk::Orientation::VERTICAL, 8)
{
    auto la = Gtk::manage(new Gtk::Label);
    la->set_markup("<b>" + Glib::Markup::escape_text(title) + "</b>");
    la->show();
    la->set_xalign(0);
    la->set_margin_start(2);
    append(*la);

    // auto fr = Gtk::manage(new Gtk::Frame);
    // fr->set_shadow_type(Gtk::SHADOW_IN);
    m_listbox = Gtk::manage(new Gtk::ListBox);
    m_listbox->set_activate_on_single_click(true);
    m_listbox->add_css_class("boxed-list");

    // listbox->set_header_func(sigc::ptr_fun(&header_func_separator));
    m_listbox->set_selection_mode(Gtk::SelectionMode::NONE);
    // fr->show_all();
    m_listbox->signal_row_activated().connect([](Gtk::ListBoxRow *lrow) {
        if (auto row = dynamic_cast<PreferencesRow *>(lrow->get_child())) {
            row->activate();
        }
    });
    append(*m_listbox);
}

void PreferencesGroup::set_placeholder(Gtk::Widget &w)
{
    m_listbox->set_placeholder(w);
}

void PreferencesGroup::add_row(PreferencesRow &row)
{
    row.show();
    m_listbox->append(row);
}


} // namespace dune3d
