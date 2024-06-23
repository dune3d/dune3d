#include "selection_texture_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"

namespace dune3d {
SelectionTextureRenderer::SelectionTextureRenderer(Canvas &c) : m_ca(c)
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

void SelectionTextureRenderer::realize()
{
    m_program = gl_create_program_from_resource(
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-texture-vertex.glsl",
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-texture-fragment.glsl",
            nullptr);
    m_vao = create_vao(m_program);

    GET_LOC(this, tex);
    GET_LOC(this, samples);
}

void SelectionTextureRenderer::render()
{
    glUseProgram(m_program);
    GL_CHECK_ERROR
    glBindVertexArray(m_vao);
    GL_CHECK_ERROR


    glActiveTexture(GL_TEXTURE2);
    glUniform1i(m_tex_loc, 2);
    glUniform1i(m_samples_loc, m_ca.get_samples());
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ca.m_selection_texture);


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    GL_CHECK_ERROR
}
} // namespace dune3d
