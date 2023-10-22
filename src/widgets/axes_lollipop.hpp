#pragma once
#include <gtkmm.h>

namespace dune3d {
class AxesLollipop : public Gtk::DrawingArea {
public:
    AxesLollipop();
    void set_angles(float a, float b);

private:
    float alpha = 0;
    float beta = 0;
    Glib::RefPtr<Pango::Layout> layout;
    float size = 5;

    void render(const Cairo::RefPtr<Cairo::Context> &cr, int w, int h);
    void create_layout();
};
} // namespace dune3d
