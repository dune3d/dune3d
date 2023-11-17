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
    size_t get_vertex_count() const override;

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_screen_scale_loc;


    static GLuint create_vao(GLuint program, GLuint &vbo_out);
};
} // namespace dune3d
