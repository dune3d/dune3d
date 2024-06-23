#pragma once
#include <epoxy/gl.h>

namespace dune3d {
class SelectionTextureRenderer {
public:
    SelectionTextureRenderer(class Canvas &c);
    void realize();
    void render();

private:
    Canvas &m_ca;

    GLuint m_program;
    GLuint m_vao;

    GLuint m_tex_loc;
    GLuint m_samples_loc;
};
} // namespace dune3d
