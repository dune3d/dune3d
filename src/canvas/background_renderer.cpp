#include "background_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include "color_palette.hpp"
#include <cmath>

namespace dune3d {
BackgroundRenderer::BackgroundRenderer(Canvas &c) : m_ca(c)
{
}

static GLuint create_vao(GLuint program)
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    float vertices[] = {//   Position
                        -1, 1, 1, 1, -1, -1, 1, -1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);

    return vao;
}

void BackgroundRenderer::realize()
{
    m_program = gl_create_program_from_resource(
            "/org/dune3d/dune3d/canvas/shaders/"
            "background-vertex.glsl",
            "/org/dune3d/dune3d/canvas/shaders/"
            "background-fragment.glsl",
            nullptr);
    m_vao = create_vao(m_program);

    GET_LOC(this, color_top);
    GET_LOC(this, color_bottom);
    GET_LOC(this, alpha);
}

void BackgroundRenderer::render()
{
    glUseProgram(m_program);
    GL_CHECK_ERROR
    glBindVertexArray(m_vao);
    GL_CHECK_ERROR

    gl_color_to_uniform_3f(m_color_top_loc, m_ca.m_appearance.get_color(ColorP::BACKGROUND_TOP));
    gl_color_to_uniform_3f(m_color_bottom_loc, m_ca.m_appearance.get_color(ColorP::BACKGROUND_BOTTOM));
    glUniform1f(m_alpha_loc, 1);
    GL_CHECK_ERROR

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    GL_CHECK_ERROR
}

void BackgroundRenderer::render_error()
{
    glUseProgram(m_program);
    GL_CHECK_ERROR
    glBindVertexArray(m_vao);
    GL_CHECK_ERROR

    gl_color_to_uniform_3f(m_color_bottom_loc, m_ca.m_appearance.get_color(ColorP::ERROR_OVERLAY));
    gl_color_to_uniform_3f(m_color_top_loc, m_ca.m_appearance.get_color(ColorP::ERROR_OVERLAY));
    glUniform1f(m_alpha_loc, .25);
    GL_CHECK_ERROR

    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GL_CHECK_ERROR
}
} // namespace dune3d
