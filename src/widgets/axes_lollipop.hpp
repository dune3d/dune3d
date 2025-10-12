#pragma once
#include <gtkmm.h>
#include <glm/gtx/quaternion.hpp>

namespace dune3d {

class AxesLollipop : public Gtk::DrawingArea {
public:
    AxesLollipop();
    void set_quat(const glm::quat &q);

    typedef sigc::signal<void(int)> type_signal_snap_to_plane; // 0: YZ, 1: XZ, 2: XY
    type_signal_snap_to_plane signal_snap_to_plane()
    {
        return m_signal_snap_to_plane;
    }

private:
    glm::quat m_quat;
    Glib::RefPtr<Pango::Layout> m_layout;
    float m_size = 5;

    type_signal_snap_to_plane m_signal_snap_to_plane;

    int get_clicked_axis(double x, double y, int w, int h) const;

    void render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h);
    void create_layout();

    void on_click(guint n_press, double x, double y);
};
} // namespace dune3d
