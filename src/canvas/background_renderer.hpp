#pragma once
#include <epoxy/gl.h>

namespace dune3d {
class BackgroundRenderer {
public:
    BackgroundRenderer(class Canvas &c);
    void realize();
    void render();

private:
    Canvas &m_ca;

    GLuint m_program;
    GLuint m_vao;

    GLuint m_color_top_loc;
    GLuint m_color_bottom_loc;
};
} // namespace dune3d
