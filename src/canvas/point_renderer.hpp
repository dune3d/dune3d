#pragma once
#include "base_renderer.hpp"

namespace dune3d {
class PointRenderer : public BaseRenderer {
public:
    PointRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    size_t get_vertex_count() const override;

    GLuint m_vao;
    GLuint m_vbo;

    GLuint m_z_offset_loc;
    static GLuint create_vao(GLuint program, GLuint &vbo_out);
};
} // namespace dune3d
