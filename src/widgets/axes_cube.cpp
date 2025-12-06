#include "axes_cube.hpp"
#include "util/color.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <vector>
#include <string>
#include <algorithm>

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

AxesCube::AxesCube()
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
    set_draw_func(sigc::mem_fun(*this, &AxesCube::render));
    setup_controllers();
}

void AxesCube::create_layout()
{
    m_layout = create_pango_layout("");
    Pango::AttrList attrs;
    auto attr = Pango::Attribute::create_attr_weight(Pango::Weight::BOLD);
    attrs.insert(attr);
    m_layout->set_attributes(attrs);
}

void AxesCube::set_quat(const glm::quat &q)
{
    m_quat = q;
    queue_draw();
}

sigc::signal<void(const glm::quat &)> AxesCube::signal_quat_changed()
{
    return m_signal_quat_changed;
}

namespace {
struct Face {
    std::vector<int> vertices;
    int id;
    std::string name;
    Color color;
    std::string label;
    std::optional<glm::quat> target_quat;
};

struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;
};
} // namespace

static Model generate_model()
{
    constexpr double S = 0.9;
    constexpr double B = 0.60;

    std::vector<glm::vec3> vertices;
    vertices.reserve(24);
    std::vector<Face> faces;
    faces.reserve(26);

    // generate vertices
    for (int i = 0; i < 8; ++i) {
        double sx = (i & 1) ? 1.0 : -1.0;
        double sy = (i & 2) ? 1.0 : -1.0;
        double sz = (i & 4) ? 1.0 : -1.0;

        vertices.push_back({sx * S, sy * B, sz * B}); // 0: X-side
        vertices.push_back({sx * B, sy * S, sz * B}); // 1: Y-side
        vertices.push_back({sx * B, sy * B, sz * S}); // 2: Z-side
    }

    int id = 0;
    Color col_gray = Color::new_from_int(200, 200, 200);

    auto add_face = [&](std::vector<int> idxs, std::string name, Color c, std::string lbl = "",
                        glm::quat quat = glm::quat()) {
        faces.push_back(
                {std::move(idxs), id++, std::move(name), c, std::move(lbl), std::make_optional(std::move(quat))});
    };

    // faces
    add_face({1 * 3 + 0, 5 * 3 + 0, 7 * 3 + 0, 3 * 3 + 0}, "−X", get_color(0, 1.0f), "−X",
             glm::quat(glm::vec3(0, -glm::pi<float>() / 2, 0)));
    add_face({0 * 3 + 0, 2 * 3 + 0, 6 * 3 + 0, 4 * 3 + 0}, "+X", get_color(0, -1.0f), "X",
             glm::quat(glm::vec3(0, glm::pi<float>() / 2, 0)));
    add_face({2 * 3 + 1, 3 * 3 + 1, 7 * 3 + 1, 6 * 3 + 1}, "−Y", get_color(1, 1.0f), "−Y",
             glm::quat(glm::vec3(glm::pi<float>() / 2, 0, 0)));
    add_face({0 * 3 + 1, 4 * 3 + 1, 5 * 3 + 1, 1 * 3 + 1}, "+Y", get_color(1, -1.0f), "Y",
             glm::quat(glm::vec3(-glm::pi<float>() / 2, 0, 0)));
    add_face({4 * 3 + 2, 6 * 3 + 2, 7 * 3 + 2, 5 * 3 + 2}, "−Z", get_color(2, 1.0f), "−Z",
             glm::quat(glm::vec3(0, glm::pi<float>(), 0)));
    add_face({0 * 3 + 2, 1 * 3 + 2, 3 * 3 + 2, 2 * 3 + 2}, "+Z", get_color(2, -1.0f), "Z", glm::quat(1, 0, 0, 0));

    // corners
    for (int i = 0; i < 8; ++i) {
        double sx = (i & 1) ? 1.0 : -1.0;
        double sy = (i & 2) ? 1.0 : -1.0;
        double sz = (i & 4) ? 1.0 : -1.0;
        std::string sname = "Corner " + std::string(sx > 0 ? "+X" : "-X") + std::string(sy > 0 ? "+Y" : "-Y")
                            + std::string(sz > 0 ? "+Z" : "-Z");

        // isometric views looking toward the corner
        glm::vec3 target_dir(sx, sy, sz);
        glm::vec3 up(0, 0, 1);
        if (std::abs(target_dir.z) > 0.9) {
            up = glm::vec3(0, 1, 0);
        }
        glm::quat corner_quat = glm::quatLookAt(glm::normalize(target_dir), up);

        if (sx * sy * sz > 0)
            add_face({i * 3 + 0, i * 3 + 2, i * 3 + 1}, sname, col_gray, "", corner_quat);
        else
            add_face({i * 3 + 2, i * 3 + 0, i * 3 + 1}, sname, col_gray, "", corner_quat);
    }

    // X-axis edges
    add_face({6 * 3 + 1, 7 * 3 + 1, 7 * 3 + 2, 6 * 3 + 2}, "Edge −Y−Z", col_gray, "",
             glm::quat(glm::vec3(glm::pi<float>() / 4, glm::pi<float>() / 1, 0)));
    add_face({2 * 3 + 2, 3 * 3 + 2, 3 * 3 + 1, 2 * 3 + 1}, "Edge −Y+Z", col_gray, "",
             glm::quat(glm::vec3(glm::pi<float>() / 4, 0, 0)));
    add_face({4 * 3 + 2, 5 * 3 + 2, 5 * 3 + 1, 4 * 3 + 1}, "Edge +Y−Z", col_gray, "",
             glm::quat(glm::vec3(-glm::pi<float>() / 4, glm::pi<float>() / 1, 0)));
    add_face({0 * 3 + 1, 1 * 3 + 1, 1 * 3 + 2, 0 * 3 + 2}, "Edge +Y+Z", col_gray, "",
             glm::quat(glm::vec3(-glm::pi<float>() / 4, 0, 0)));

    // Y-axis edges
    add_face({5 * 3 + 2, 7 * 3 + 2, 7 * 3 + 0, 5 * 3 + 0}, "Edge −X−Z", col_gray, "",
             glm::quat(glm::vec3(0, -glm::pi<float>() * 3 / 4, 0)));
    add_face({1 * 3 + 0, 3 * 3 + 0, 3 * 3 + 2, 1 * 3 + 2}, "Edge −X+Z", col_gray, "",
             glm::quat(glm::vec3(0, -glm::pi<float>() / 4, 0)));
    add_face({4 * 3 + 0, 6 * 3 + 0, 6 * 3 + 2, 4 * 3 + 2}, "Edge +X−Z", col_gray, "",
             glm::quat(glm::vec3(0, glm::pi<float>() * 3 / 4, 0)));
    add_face({0 * 3 + 2, 2 * 3 + 2, 2 * 3 + 0, 0 * 3 + 0}, "Edge +X+Z", col_gray, "",
             glm::quat(glm::vec3(0, glm::pi<float>() / 4, 0)));

    // Z-axis edges
    add_face({3 * 3 + 0, 7 * 3 + 0, 7 * 3 + 1, 3 * 3 + 1}, "Edge −X−Y", col_gray, "",
             glm::quat(glm::vec3(glm::pi<float>() / 4, -glm::pi<float>() / 2, 0)));
    add_face({1 * 3 + 1, 5 * 3 + 1, 5 * 3 + 0, 1 * 3 + 0}, "Edge −X+Y", col_gray, "",
             glm::quat(glm::vec3(-glm::pi<float>() / 4, -glm::pi<float>() / 2, 0)));
    add_face({2 * 3 + 1, 6 * 3 + 1, 6 * 3 + 0, 2 * 3 + 0}, "Edge +X−Y", col_gray, "",
             glm::quat(glm::vec3(0, glm::pi<float>() / 2, -glm::pi<float>() / 4)));
    add_face({0 * 3 + 0, 4 * 3 + 0, 4 * 3 + 1, 0 * 3 + 1}, "Edge +X+Y", col_gray, "",
             glm::quat(glm::vec3(0, glm::pi<float>() / 2, glm::pi<float>() / 4)));

    // Axis vertices (+X+Y+Z corner only)
    constexpr float S_axis = 0.9f;
    constexpr float offset_factor = 1.08f;
    constexpr float axis_length = 0.55f;
    const float sx = 1.0f, sy = 1.0f, sz = 1.0f;
    glm::vec3 origin = glm::vec3(sx * S_axis, sy * S_axis, sz * S_axis) * offset_factor;
    vertices.push_back(origin);
    vertices.push_back(origin - glm::vec3(sx * axis_length, 0.0f, 0.0f));
    vertices.push_back(origin - glm::vec3(0.0f, sy * axis_length, 0.0f));
    vertices.push_back(origin - glm::vec3(0.0f, 0.0f, sz * axis_length));

    return Model{vertices, faces};
}

static const auto &get_cached_model()
{
    static const auto model = generate_model();
    return model;
}

static bool is_face_visible(const std::vector<glm::vec3> &m_transformed_vertices, const std::vector<int> &face_vertices)
{
    if (face_vertices.size() < 3)
        return false;
    const glm::vec3 &v0 = m_transformed_vertices[face_vertices[0]];
    const glm::vec3 &v1 = m_transformed_vertices[face_vertices[1]];
    const glm::vec3 &v2 = m_transformed_vertices[face_vertices[2]];
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::cross(edge1, edge2);
    return normal.z < 0;
}

void AxesCube::update_transformed_vertices()
{
    const glm::quat corrected_view_quat =
            glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::inverse(m_quat);

    if (m_transformed_vertices.empty() || m_quat != m_cached_quat || m_width != m_cached_width
        || m_height != m_cached_height) {

        const auto &vertices = get_cached_model().vertices;
        const float sc = (std::min(m_width, m_height) / 2.0f) - m_size;

        if (m_transformed_vertices.size() != vertices.size()) {
            m_transformed_vertices.resize(vertices.size());
        }

        for (size_t i = 0; i < vertices.size(); i++) {
            m_transformed_vertices[i] = glm::rotate(corrected_view_quat, vertices[i]) * sc;
        }

        m_cached_quat = m_quat;
        m_cached_width = m_width;
        m_cached_height = m_height;
    }
}

void AxesCube::setup_controllers()
{
    auto motion_controller = Gtk::EventControllerMotion::create();
    motion_controller->signal_motion().connect([this](double x, double y) {
        m_last_x = x;
        m_last_y = y;
        int old_hovered = m_hovered_face;
        m_hovered_face = get_face_at_position(x, y);
        if (old_hovered != m_hovered_face) {
            queue_draw();
        }
    });
    motion_controller->signal_leave().connect([this] {
        if (m_hovered_face != -1) {
            m_hovered_face = -1;
            queue_draw();
        }
    });
    add_controller(motion_controller);

    auto click_controller = Gtk::GestureClick::create();
    click_controller->set_button(1);
    click_controller->signal_pressed().connect([this](int n_press, double x, double y) {
        int face_id = get_face_at_position(x, y);
        if (face_id >= 0) {
            const auto &faces = get_cached_model().faces;
            if (face_id < (int)faces.size()) {
                const auto &face = faces[face_id];

                if (face.target_quat) {
                    m_signal_quat_changed.emit(*face.target_quat);
                }
            }
        }
    });
    add_controller(click_controller);
}

int AxesCube::get_face_at_position(double x, double y) const
{
    if (m_width == 0 || m_height == 0)
        return -1;

    const glm::vec2 p(x - m_width / 2.0f, y - m_height / 2.0f);
    const auto &faces = get_cached_model().faces;

    int best_face = -1;
    float min_z = 1e9f;

    for (const auto &face : faces) {
        if (!is_face_visible(m_transformed_vertices, face.vertices))
            continue;

        bool hit = true;
        const size_t n = face.vertices.size();
        for (size_t i = 0; i < n; i++) {
            const auto &v1 = m_transformed_vertices[face.vertices[i]];
            const auto &v2 = m_transformed_vertices[face.vertices[(i + 1) % n]];
            if ((v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x) > 0) {
                hit = false;
                break;
            }
        }

        if (hit) {
            float avg_z = 0;
            for (int idx : face.vertices)
                avg_z += m_transformed_vertices[idx].z;
            avg_z /= n;
            if (avg_z < min_z) {
                min_z = avg_z;
                best_face = face.id;
            }
        }
    }
    return best_face;
}

void AxesCube::render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h)
{
    m_width = w;
    m_height = h;

    update_transformed_vertices();

    cr->translate(w / 2.0, h / 2.0);
    cr->set_line_width(1.0);
    cr->set_line_join(Cairo::Context::LineJoin::ROUND);

    const auto &faces = get_cached_model().faces;

    struct VisibleFace {
        const Face *face;
        float depth;
    };
    std::vector<VisibleFace> visible_faces;
    visible_faces.reserve(faces.size());

    for (const auto &face : faces) {
        if (is_face_visible(m_transformed_vertices, face.vertices)) {
            float avg_z = 0;
            for (int idx : face.vertices)
                avg_z += m_transformed_vertices[idx].z;
            avg_z /= face.vertices.size();
            visible_faces.push_back({&face, avg_z});
        }
    }

    std::sort(visible_faces.begin(), visible_faces.end(),
              [](const VisibleFace &a, const VisibleFace &b) { return a.depth < b.depth; });

    for (const auto &vf : visible_faces) {
        const Face &face = *vf.face;
        Color color = face.color;

        if (m_hovered_face == face.id) {
            color.r = std::min(1.0f, color.r * 1.4f);
            color.g = std::min(1.0f, color.g * 1.4f);
            color.b = std::min(1.0f, color.b * 1.4f);
        }

        const auto &v0 = m_transformed_vertices[face.vertices[0]];
        cr->move_to(v0.x, v0.y);
        for (size_t i = 1; i < face.vertices.size(); ++i) {
            const auto &v = m_transformed_vertices[face.vertices[i]];
            cr->line_to(v.x, v.y);
        }
        cr->close_path();

        cr->set_source_rgba(color.r, color.g, color.b, 0.7);
        cr->fill_preserve();

        cr->set_source_rgb(0, 0, 0);
        cr->stroke();

        if (!face.label.empty()) {
            m_layout->set_text(face.label);
            auto ext = m_layout->get_pixel_logical_extents();

            const glm::vec3 &v0 = m_transformed_vertices[face.vertices[0]];
            const glm::vec3 &v1 = m_transformed_vertices[face.vertices[1]];
            const glm::vec3 &v2 = m_transformed_vertices[face.vertices[2]];
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            float dot_prod_view = -normal.z;

            static const float LABEL_BASE_SCALE = 1.0f;
            static const float LABEL_MIN_VISIBILITY_THRESHOLD = 0.1f;
            static const float LABEL_MAX_SCALE_FACTOR = 1.5f;
            static const float LABEL_STEADY_DOT_PRODUCT_THRESHOLD = 0.5f;

            float current_scale = 0.0f;
            if (dot_prod_view > LABEL_MIN_VISIBILITY_THRESHOLD) {
                if (dot_prod_view >= LABEL_STEADY_DOT_PRODUCT_THRESHOLD) {
                    current_scale = LABEL_BASE_SCALE;
                }
                else {
                    float scale_range = LABEL_STEADY_DOT_PRODUCT_THRESHOLD - LABEL_MIN_VISIBILITY_THRESHOLD;
                    if (scale_range > 0) {
                        float normalized_dot = (dot_prod_view - LABEL_MIN_VISIBILITY_THRESHOLD) / scale_range;
                        current_scale = LABEL_BASE_SCALE * normalized_dot;
                    }
                    else {
                        current_scale = 0.0f;
                    }
                }
                current_scale = std::min(current_scale, LABEL_MAX_SCALE_FACTOR);
            }

            if (current_scale > 0.01f) {
                float center_x = 0, center_y = 0;
                for (int idx : face.vertices) {
                    center_x += m_transformed_vertices[idx].x;
                    center_y += m_transformed_vertices[idx].y;
                }
                center_x /= face.vertices.size();
                center_y /= face.vertices.size();

                cr->save();
                cr->translate(center_x, center_y);
                cr->scale(current_scale, current_scale);

                cr->set_source_rgb(0, 0, 0);
                cr->move_to(-ext.get_width() / 2.0, -ext.get_height() / 2.0);
                m_layout->show_in_cairo_context(cr);
                cr->restore();
            }
        }
    }

    // corner axes display
    const int AXIS_BASE = 8 * 3; // vertices[24..27]
    if ((int)m_transformed_vertices.size() >= AXIS_BASE + 4) {
        cr->set_line_width(2.0);
        cr->set_line_cap(Cairo::Context::LineCap::ROUND);

        const int base = AXIS_BASE;
        for (int ax = 0; ax < 3; ++ax) {
            const auto &o = m_transformed_vertices[base + 0];
            const auto &e = m_transformed_vertices[base + (ax + 1)];
            const Color &col = dune3d::get_color(ax, 1.0f);
            cr->set_source_rgba(col.r, col.g, col.b, 0.8f);
            cr->move_to(o.x, o.y);
            cr->line_to(e.x, e.y);
            cr->stroke();
        }
    }
}

} // namespace dune3d
