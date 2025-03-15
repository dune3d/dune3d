#pragma once
#include "base_renderer.hpp"

namespace dune3d {
class GlyphRenderer : public BaseRenderer {
public:
    GlyphRenderer(class Canvas &c);
    void realize();
    void render();
    void push();

private:
    GLuint m_vao;
    GLuint m_vbo;

    GLuint m_screen_loc;
    GLuint m_msdf_loc;
    GLuint m_scale_factor_loc;
    GLuint m_texture_glyph;

    static GLuint create_vao(GLuint program, GLuint &vbo_out);
};
} // namespace dune3d
