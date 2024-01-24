#include "glyph_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include "bitmap_font_util.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace dune3d {
GlyphRenderer::GlyphRenderer(Canvas &c) : BaseRenderer(c, Canvas::VertexType::GLYPH)
{
}

GLuint GlyphRenderer::create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint origin_index = glGetAttribLocation(program, "origin");
    GLuint shift_index = glGetAttribLocation(program, "shift");
    GLuint scale_index = glGetAttribLocation(program, "scale");
    GLuint bits_index = glGetAttribLocation(program, "bits");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    Canvas::GlyphVertex vertices[] = {
            //   Position
            {
                    0,
            },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(origin_index);
    glVertexAttribPointer(origin_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::GlyphVertex), 0);
    GL_CHECK_ERROR
    glEnableVertexAttribArray(shift_index);
    glVertexAttribPointer(shift_index, 2, GL_FLOAT, GL_FALSE, sizeof(Canvas::GlyphVertex),
                          (void *)offsetof(Canvas::GlyphVertex, xs));
    GL_CHECK_ERROR
    glEnableVertexAttribArray(scale_index);
    glVertexAttribPointer(scale_index, 1, GL_FLOAT, GL_FALSE, sizeof(Canvas::GlyphVertex),
                          (void *)offsetof(Canvas::GlyphVertex, scale));
    GL_CHECK_ERROR
    glEnableVertexAttribArray(bits_index);
    glVertexAttribIPointer(bits_index, 1, GL_UNSIGNED_INT, sizeof(Canvas::GlyphVertex),
                           (void *)offsetof(Canvas::GlyphVertex, bits));
    GL_CHECK_ERROR
    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_INT, sizeof(Canvas::GlyphVertex),
                           (void *)offsetof(Canvas::GlyphVertex, flags));
    GL_CHECK_ERROR
    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

void GlyphRenderer::realize()
{
    glGenTextures(1, &m_texture_glyph);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture_glyph);
    bitmap_font::load_texture();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    m_program = gl_create_program_from_resource("/org/dune3d/dune3d/canvas/shaders/glyph-vertex.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/glyph-fragment.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/glyph-geometry.glsl");
    m_vao = create_vao(m_program, m_vbo);

    realize_base();

    GET_LOC(this, screen);
    GET_LOC(this, msdf);
    GET_LOC(this, scale_factor);
}

void GlyphRenderer::push()
{

    m_ca.m_n_glyphs = m_ca.m_glyphs.size();

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Canvas::GlyphVertex) * m_ca.m_n_glyphs, m_ca.m_glyphs.data(), GL_STATIC_DRAW);
}

void GlyphRenderer::render()
{
    if (!m_ca.m_n_glyphs)
        return;
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(m_msdf_loc, 0);
    glUniform1f(m_scale_factor_loc, m_ca.m_scale_factor);
    glBindTexture(GL_TEXTURE_2D, m_texture_glyph);


    glUniformMatrix3fv(m_screen_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_screenmat));
    load_uniforms();

    glDrawArrays(GL_POINTS, 0, m_ca.m_n_glyphs);
}

size_t GlyphRenderer::get_vertex_count() const
{
    return m_ca.m_n_glyphs;
}


} // namespace dune3d
