#include "axes_lollipop.hpp"
#include "color.hpp"
#include <glm/gtx/rotate_vector.hpp>

namespace dune3d {

static const std::array<std::string, 3> s_xyz = {"X", "Y", "Z"};

static const Color &get_color(unsigned int ax, float z)
{
    static const std::array<Color, 3> ax_colors_pos = {
            Color::new_from_int(255, 54, 83),
            Color::new_from_int(138, 219, 0),
            Color::new_from_int(44, 142, 254),
    };
    static const std::array<Color, 3> ax_colors_neg = {
            Color::new_from_int(155, 57, 7),
            Color::new_from_int(98, 137, 34),
            Color::new_from_int(51, 100, 155),
    };
    if (z >= -.001)
        return ax_colors_pos.at(ax);
    else
        return ax_colors_neg.at(ax);
}

AxesLollipop::AxesLollipop()
{
    create_layout();
    for (unsigned int ax = 0; ax < 3; ax++) {
        layout->set_text(s_xyz.at(ax));
        auto ext = layout->get_pixel_logical_extents();
        size = std::max(size, (float)ext.get_width());
        size = std::max(size, (float)ext.get_height());
    }
    set_content_height(100);
    set_content_width(100);
    set_draw_func(sigc::mem_fun(*this, &AxesLollipop::render));
    // signal_screen_changed().connect([this](const Glib::RefPtr<Gdk::Screen> &screen) { create_layout(); });
}

void AxesLollipop::create_layout()
{
    layout = create_pango_layout("");
    Pango::AttrList attrs;
    auto attr = Pango::Attribute::create_attr_weight(Pango::Weight::BOLD);
    attrs.insert(attr);
    layout->set_attributes(attrs);
}

void AxesLollipop::set_angles(float a, float b)
{
    alpha = a;
    beta = b;
    queue_draw();
}

void AxesLollipop::render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h)
{
    const float sc = (std::min(w, h) / 2) - size;
    cr->translate(w / 2, h / 2);
    cr->set_line_width(2);
    std::vector<std::pair<unsigned int, glm::vec3>> pts;
    for (unsigned int ax = 0; ax < 3; ax++) {
        const glm::vec3 v(ax == 0, ax == 1, ax == 2);
        const auto vt = glm::rotateX(glm::rotateZ(v, alpha), beta) * sc;
        pts.emplace_back(ax, vt);
    }

    std::sort(pts.begin(), pts.end(), [](const auto &a, const auto &b) { return a.second.z < b.second.z; });

    for (const auto &[ax, vt] : pts) {
        cr->move_to(0, 0);
        cr->line_to(vt.x, -vt.y);
        const auto &c = dune3d::get_color(ax, vt.z / sc);
        cr->set_source_rgb(c.r, c.g, c.b);
        cr->stroke();
    }
    for (const auto &[ax, vt] : pts) {
        const auto &c = dune3d::get_color(ax, vt.z / sc);
        cr->set_source_rgb(c.r, c.g, c.b);

        cr->arc(vt.x, -vt.y, size * .6, 0.0, 2.0 * M_PI);
        cr->fill();

        layout->set_text(s_xyz.at(ax));
        auto ext = layout->get_pixel_logical_extents();
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(vt.x - ext.get_width() / 2, -vt.y - ext.get_height() / 2);
        layout->show_in_cairo_context(cr);
    }
}

} // namespace dune3d
