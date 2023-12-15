#pragma once
#include <gtkmm.h>
#include <glm/gtx/quaternion.hpp>

namespace dune3d {
class AxesLollipop : public Gtk::DrawingArea {
public:
    AxesLollipop();
    void set_quat(const glm::quat &q);

private:
    glm::quat m_quat;
    Glib::RefPtr<Pango::Layout> m_layout;
    float m_size = 5;

    void render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h);
    void create_layout();
};
} // namespace dune3d
