#pragma once
#include "base_renderer.hpp"

namespace dune3d {
class LineRenderer : public BaseRenderer {
public:
    LineRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_screen_scale_loc;
    GLuint m_screen_loc;
    GLuint m_line_width_loc;


    static GLuint create_vao(GLuint program, GLuint &vbo_out);
};
} // namespace dune3d
