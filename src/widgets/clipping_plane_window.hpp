#pragma once
#include <gtkmm.h>
#include "canvas/clipping_planes.hpp"
#include "util/changeable.hpp"

namespace dune3d {

class ClippingPlaneWindow : public Gtk::Window, public Changeable {
public:
    ClippingPlaneWindow();

    ClippingPlanes get_planes() const;

    void toggle_global();

private:
    class ClippingPlaneBox;
    ClippingPlaneBox *m_box_x = nullptr;
    ClippingPlaneBox *m_box_y = nullptr;
    ClippingPlaneBox *m_box_z = nullptr;

    Gtk::Switch *m_global_sw = nullptr;
};

} // namespace dune3d
