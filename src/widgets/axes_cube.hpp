#pragma once
#include <gtkmm.h>
#include <glm/gtx/quaternion.hpp>
#include <vector>

namespace dune3d {
class AxesCube : public Gtk::DrawingArea {
public:
    AxesCube();
    void set_quat(const glm::quat &q);

    sigc::signal<void(const glm::quat &)> signal_quat_changed();

private:
    glm::quat m_quat;
    Glib::RefPtr<Pango::Layout> m_layout;
    float m_size = 5;
    int m_hovered_face = -1;
    double m_last_x = 0;
    double m_last_y = 0;
    int m_width = 0;
    int m_height = 0;

    std::vector<glm::vec3> m_transformed_vertices;
    glm::quat m_cached_quat{1, 0, 0, 0};
    int m_cached_width = 0;
    int m_cached_height = 0;

    sigc::signal<void(const glm::quat &)> m_signal_quat_changed;

    void render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h);
    void create_layout();
    void setup_controllers();
    int get_face_at_position(double x, double y) const;
    void update_transformed_vertices();
};
} // namespace dune3d
