#pragma once
#include "base_renderer.hpp"

namespace dune3d {
class IconRenderer : public BaseRenderer {
public:
    IconRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    size_t get_vertex_count() const override;

    GLuint m_vao;
    GLuint m_vbo;

    GLuint m_screen_loc;
    GLuint m_tex_loc;
    GLuint m_icon_size_loc;
    GLuint m_icon_border_loc;
    GLuint m_texture_size_loc;
    GLuint m_texture_icon;
    GLuint m_scale_factor_loc;

    static GLuint create_vao(GLuint program, GLuint &vbo_out);

    unsigned int m_texture_size;
};
} // namespace dune3d
