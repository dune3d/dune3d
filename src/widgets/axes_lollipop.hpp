#pragma once
#include <gtkmm.h>
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

class Canvas;

class AxesLollipop : public Gtk::DrawingArea {
public:
    AxesLollipop();
    void set_quat(const glm::quat &q);
    
    // Add method to set the canvas for view manipulation
    void set_canvas(Canvas &canvas);

private:
    glm::quat m_quat;
    Glib::RefPtr<Pango::Layout> m_layout;
    float m_size = 5;
    
    // Add canvas pointer for view manipulation
    Canvas *m_canvas = nullptr;
    
    // Add methods for snapping to planes
    void snap_to_xy_plane();
    void snap_to_xz_plane();
    void snap_to_yz_plane();
    
    // Add method to determine which axis was clicked
    int get_clicked_axis(double x, double y, int w, int h) const;

    void render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h);
    void create_layout();
    
    // Add event handling
    void on_click(guint n_press, double x, double y);
};
} // namespace dune3d
