#include "preferences_window_canvas.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/changeable.hpp"
#include "nlohmann/json.hpp"
#include "canvas/color_palette.hpp"
#include "color_presets.hpp"
#include <format>

namespace dune3d {


static const std::map<ColorP, std::string> color_names = {
        {ColorP::ENTITY, "Entity"},
        {ColorP::CONSTRAINT, "Constraint"},
        {ColorP::BACKGROUND_BOTTOM, "Background bottom"},
        {ColorP::BACKGROUND_TOP, "Background top"},
        {ColorP::POINT, "Point"},
        {ColorP::CONSTRUCTION_POINT, "Construction point"},
        {ColorP::SOLID_MODEL, "Solid model"},
        {ColorP::OTHER_BODY_SOLID_MODEL, "Solid model (other body)"},
        {ColorP::CONSTRUCTION_ENTITY, "Construction entity"},
        {ColorP::INACTIVE_ENTITY, "Inactive entity"},
        {ColorP::INACTIVE_POINT, "Inactive point"},
        {ColorP::HOVER, "Hover"},
        {ColorP::SELECTED, "Selected"},
        {ColorP::SELECTED_HOVER, "Selected hover"},
        {ColorP::HIGHLIGHT, "Highlight"},
        {ColorP::SELECTION_BOX, "Selection box"},
};

class ColorEditorPalette : public Gtk::Box {
public:
    ColorEditorPalette(Appearance &a, Preferences &p, ColorP c);
    Color get_color();
    void set_color(const Color &c);

protected:
    Gtk::DrawingArea *m_colorbox = nullptr;

    Appearance &m_apperance;
    Preferences &m_prefs;
    const ColorP m_color;
};

ColorEditorPalette::ColorEditorPalette(Appearance &a, Preferences &p, ColorP c)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL, 5), m_apperance(a), m_prefs(p), m_color(c)
{
    auto la = Gtk::make_managed<Gtk::Label>(color_names.at(m_color));
    la->set_xalign(0);
    Gdk::RGBA rgba;
    auto co = get_color();
    rgba.set_red(co.r);
    rgba.set_green(co.g);
    rgba.set_blue(co.b);
    rgba.set_alpha(1);
    m_colorbox = Gtk::manage(new Gtk::DrawingArea);
    m_colorbox->set_content_height(20);
    m_colorbox->set_content_width(20);
    m_colorbox->set_draw_func([this](const Cairo::RefPtr<Cairo::Context> &cr, int w, int h) {
        auto c = get_color();
        cr->set_source_rgb(c.r, c.g, c.b);
        cr->paint();
    });
    append(*m_colorbox);
    append(*la);
    set_margin_start(5);
    set_margin_end(5);
    set_valign(Gtk::Align::CENTER);
}

Color ColorEditorPalette::get_color()
{
    return m_apperance.get_color(m_color);
}

void ColorEditorPalette::set_color(const Color &c)
{
    m_apperance.colors[m_color] = c;
    m_colorbox->queue_draw();
    m_prefs.signal_changed().emit();
}

static void spinbutton_set_px(Gtk::SpinButton &sp)
{
    sp.signal_output().connect(
            [&sp] {
                sp.set_text(std::format("{:.1f} px", sp.get_value()));
                return true;
            },
            true);
}

CanvasPreferencesEditor::CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                 Preferences &prefs)
    : Gtk::Box(cobject), m_preferences(prefs), m_canvas_preferences(m_preferences.canvas)
{
    m_colors_box = x->get_widget<Gtk::FlowBox>("colors_box");
    // m_color_chooser = GTK_COLOR_CHOOSER_WIDGET(x->get_widget<Gtk::Widget>("color_chooser")->gobj());
    // m_color_chooser = x->get_object<Gtk::ColorChooser>("color_chooser");
    m_color_chooser = Glib::wrap(GTK_COLOR_CHOOSER(gtk_builder_get_object(x->gobj(), "color_chooser")), true);
    m_colors_revealer = x->get_widget<Gtk::Revealer>("colors_revealer");
    m_theme_variant_auto_button = x->get_widget<Gtk::ToggleButton>("theme_variant_auto_button");
    m_theme_variant_light_button = x->get_widget<Gtk::ToggleButton>("theme_variant_light_button");
    m_theme_variant_dark_button = x->get_widget<Gtk::ToggleButton>("theme_variant_dark_button");
    m_theme_customize_button = x->get_widget<Gtk::Button>("theme_customize_button");

    {
        auto line_width_sp = x->get_widget<Gtk::SpinButton>("line_width_sp");
        spinbutton_set_px(*line_width_sp);
        line_width_sp->set_value(m_canvas_preferences.appearance.line_width);
        line_width_sp->signal_value_changed().connect([this, line_width_sp] {
            m_canvas_preferences.appearance.line_width = line_width_sp->get_value();
            m_preferences.signal_changed().emit();
        });
    }
    {
        auto msaa_dropdown = x->get_widget<Gtk::DropDown>("msaa_dropdown");
        //
        auto items = Gtk::StringList::create();
        items->append("Off");
        m_msaa_settings.push_back(0);
        for (int i = 1; i < 5; i *= 2) {
            items->append(std::to_string(i) + "× MSAA");
            m_msaa_settings.push_back(i);
        }
        msaa_dropdown->set_model(items);
        msaa_dropdown->set_selected(std::ranges::find(m_msaa_settings, m_canvas_preferences.appearance.msaa)
                                    - m_msaa_settings.begin());

        msaa_dropdown->property_selected().signal_changed().connect([this, msaa_dropdown] {
            int msaa = m_msaa_settings.at(msaa_dropdown->get_selected());
            m_canvas_preferences.appearance.msaa = msaa;
            m_preferences.signal_changed().emit();
        });
    }

    {
        m_theme_dropdown = x->get_widget<Gtk::DropDown>("theme_dropdown");
        //
        auto items = Gtk::StringList::create();

        for (const auto &[theme_name, theme] : color_themes) {
            items->append(theme_name);
            m_themes.push_back(theme_name);
        }

        items->append("Custom");
        m_themes.push_back("Custom");
        m_theme_dropdown->set_model(items);
        {
            auto theme = m_canvas_preferences.theme;
            if (!color_themes.contains(theme))
                theme = "Default";
            m_theme_dropdown->set_selected(std::ranges::find(m_themes, m_canvas_preferences.theme) - m_themes.begin());
        }
        m_theme_dropdown->property_selected().signal_changed().connect([this] {
            const auto &theme = m_themes.at(m_theme_dropdown->get_selected());
            m_canvas_preferences.theme = theme;
            update_colors_revealer();
            m_preferences.signal_changed().emit();
        });
        update_colors_revealer();
    }
    m_theme_customize_button->signal_clicked().connect([this] {
        auto dark = Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().get_value();
        switch (m_preferences.canvas.theme_variant) {
        case CanvasPreferences::ThemeVariant::AUTO:
            break;
        case CanvasPreferences::ThemeVariant::DARK:
            dark = true;
            break;
        case CanvasPreferences::ThemeVariant::LIGHT:
            dark = false;
            break;
        }

        auto &colors = color_themes.at(m_preferences.canvas.theme).get(dark);
        for (auto &[color_name, color] : colors) {
            if (m_color_editors.contains(color_name))
                m_color_editors.at(color_name)->set_color(color);
        }
        m_theme_dropdown->set_selected(m_themes.size() - 1); // custom
        update_color_chooser();
    });
    {
        std::map<CanvasPreferences::ThemeVariant, Gtk::ToggleButton *> buttons = {
                {CanvasPreferences::ThemeVariant::AUTO, m_theme_variant_auto_button},
                {CanvasPreferences::ThemeVariant::LIGHT, m_theme_variant_light_button},
                {CanvasPreferences::ThemeVariant::DARK, m_theme_variant_dark_button},
        };
        bind_widget<CanvasPreferences::ThemeVariant>(buttons, m_canvas_preferences.theme_variant,
                                                     [this](auto v) { m_preferences.signal_changed().emit(); });
    }

    for (const auto &[color, name] : color_names) {
        auto ed = Gtk::make_managed<ColorEditorPalette>(m_canvas_preferences.appearance, m_preferences, color);
        m_colors_box->append(*ed);
        m_color_editors.emplace(color, ed);
    }

    m_color_chooser_conn = m_color_chooser->property_rgba().signal_changed().connect([this] {
        auto sel = m_colors_box->get_selected_children();
        if (sel.size() != 1)
            return;
        auto it = dynamic_cast<ColorEditorPalette *>(sel.front()->get_child());
        Color c;
        auto rgba = m_color_chooser->get_rgba();
        c.r = rgba.get_red();
        c.g = rgba.get_green();
        c.b = rgba.get_blue();
        it->set_color(c);
    });

    m_colors_box->signal_selected_children_changed().connect(
            sigc::mem_fun(*this, &CanvasPreferencesEditor::update_color_chooser));

    m_colors_box->select_child(*m_colors_box->get_child_at_index(0));
}

void CanvasPreferencesEditor::update_colors_revealer()
{
    const auto &theme = m_themes.at(m_theme_dropdown->get_selected());
    const auto is_custom = theme == "Custom";
    m_colors_revealer->set_reveal_child(is_custom);
    m_theme_variant_auto_button->set_sensitive(!is_custom);
    m_theme_variant_light_button->set_sensitive(!is_custom);
    m_theme_variant_dark_button->set_sensitive(!is_custom);
    m_theme_customize_button->set_sensitive(!is_custom);
}

void CanvasPreferencesEditor::update_color_chooser()
{
    auto sel = m_colors_box->get_selected_children();
    if (sel.size() != 1)
        return;
    auto it = dynamic_cast<ColorEditorPalette *>(sel.front()->get_child());
    m_color_chooser_conn.block();
    Gdk::RGBA rgba;
    m_color_chooser->set_rgba(rgba); // fixes things...
    auto c = it->get_color();
    rgba.set_rgba(c.r, c.g, c.b, 1);
    m_color_chooser->set_rgba(rgba);
    m_color_chooser_conn.unblock();
}

CanvasPreferencesEditor *CanvasPreferencesEditor::create(Preferences &prefs)
{
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/dune3d/dune3d/preferences/canvas.ui");
    auto w = Gtk::Builder::get_widget_derived<CanvasPreferencesEditor>(x, "box", prefs);
    w->reference();
    return w;
}

} // namespace dune3d
