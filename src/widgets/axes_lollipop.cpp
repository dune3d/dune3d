#include "axes_lollipop.hpp"
#include "canvas/canvas.hpp"
#include "util/color.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

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
        m_layout->set_text(s_xyz.at(ax));
        auto ext = m_layout->get_pixel_logical_extents();
        m_size = std::max(m_size, (float)ext.get_width());
        m_size = std::max(m_size, (float)ext.get_height());
    }
    set_content_height(100);
    set_content_width(100);
    set_draw_func(sigc::mem_fun(*this, &AxesLollipop::render));
    
    // Make the widget focusable and clickable
    set_can_focus(true);
    set_focusable(true);
    
    // Add click event controller
    auto click_controller = Gtk::GestureClick::create();
    click_controller->set_button(1); // Left click
    click_controller->signal_pressed().connect(sigc::mem_fun(*this, &AxesLollipop::on_click));
    add_controller(click_controller);
}

void AxesLollipop::create_layout()
{
    m_layout = create_pango_layout("");
    Pango::AttrList attrs;
    auto attr = Pango::Attribute::create_attr_weight(Pango::Weight::BOLD);
    attrs.insert(attr);
    m_layout->set_attributes(attrs);
}

void AxesLollipop::set_quat(const glm::quat &q)
{
    m_quat = q;
    queue_draw();
}

void AxesLollipop::set_canvas(Canvas &canvas)
{
    m_canvas = &canvas;
}

int AxesLollipop::get_clicked_axis(double x, double y, int w, int h) const
{
    const float sc = (std::min(w, h) / 2) - m_size;
    const float center_x = w / 2.0f;
    const float center_y = h / 2.0f;
    
    // Transform screen coordinates to widget coordinates
    const float widget_x = x - center_x;
    const float widget_y = center_y - y; // Invert Y axis
    
    std::vector<std::pair<unsigned int, glm::vec3>> pts;
    for (unsigned int ax = 0; ax < 3; ax++) {
        const glm::vec3 v(ax == 0, ax == 1, ax == 2);
        const auto vt = glm::rotate(glm::inverse(m_quat), v) * sc;
        pts.emplace_back(ax, vt);
    }
    
    // Find the closest axis endpoint to the click position
    float min_distance = std::numeric_limits<float>::max();
    int closest_axis = -1;
    
    for (const auto &[ax, vt] : pts) {
        const float dx = widget_x - vt.x;
        const float dy = widget_y - vt.y;
        const float distance = std::sqrt(dx*dx + dy*dy);
        
        // Check if click is within a reasonable distance of the axis endpoint
        if (distance < m_size && distance < min_distance) {
            min_distance = distance;
            closest_axis = ax;
        }
    }
    
    return closest_axis;
}

void AxesLollipop::on_click(guint n_press, double x, double y)
{
    if (n_press == 2 && m_canvas) { // Double click
        int w = get_width();
        int h = get_height();
        
        int clicked_axis = get_clicked_axis(x, y, w, h);
        
        if (clicked_axis >= 0) {
            switch (clicked_axis) {
                case 0: // X axis - snap to YZ plane
                    snap_to_yz_plane();
                    break;
                case 1: // Y axis - snap to XZ plane
                    snap_to_xz_plane();
                    break;
                case 2: // Z axis - snap to XY plane
                    snap_to_xy_plane();
                    break;
            }
        }
    }
}

void AxesLollipop::snap_to_xy_plane()
{
    if (m_canvas) {
        // Snap to XY plane (looking down Z axis)
        glm::quat q = glm::angleAxis(0.0f, glm::vec3(1, 0, 0));
        m_canvas->animate_to_cam_quat(q);
    }
}

void AxesLollipop::snap_to_xz_plane()
{
    if (m_canvas) {
        // Snap to XZ plane (looking down Y axis)
        glm::quat q = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        m_canvas->animate_to_cam_quat(q);
    }
}

void AxesLollipop::snap_to_yz_plane()
{
    if (m_canvas) {
        // Snap to YZ plane (looking down X axis)
        glm::quat q = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0));
        m_canvas->animate_to_cam_quat(q);
    }
}

void AxesLollipop::render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h)
{
    const float sc = (std::min(w, h) / 2) - m_size;
    cr->translate(w / 2, h / 2);
    cr->set_line_width(2);
    std::vector<std::pair<unsigned int, glm::vec3>> pts;
    for (unsigned int ax = 0; ax < 3; ax++) {
        const glm::vec3 v(ax == 0, ax == 1, ax == 2);
        const auto vt = glm::rotate(glm::inverse(m_quat), v) * sc;
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

        cr->arc(vt.x, -vt.y, m_size * .6, 0.0, 2.0 * M_PI);
        cr->fill();

        m_layout->set_text(s_xyz.at(ax));
        auto ext = m_layout->get_pixel_logical_extents();
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(vt.x - ext.get_width() / 2, -vt.y - ext.get_height() / 2);
        m_layout->show_in_cairo_context(cr);
    }
}

} // namespace dune3d
