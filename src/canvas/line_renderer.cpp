#include "line_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "color_palette.hpp"

namespace dune3d {

LineRenderer::LineRenderer(Canvas &ca) : BaseRenderer(ca, Canvas::VertexType::LINE)
{
}

GLuint LineRenderer::create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint p1_index = glGetAttribLocation(program, "p1");
    GLuint p2_index = glGetAttribLocation(program, "p2");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    Canvas::LineVertex vertices[] = {
            //   Position
            {0, 0, 0, 0, 0, 10},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(p1_index);
    glVertexAttribPointer(p1_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::LineVertex), 0);
    glEnableVertexAttribArray(p2_index);
    glVertexAttribPointer(p2_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::LineVertex),
                          (void *)offsetof(Canvas::LineVertex, x2));
    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_INT, sizeof(Canvas::LineVertex),
                           (void *)offsetof(Canvas::LineVertex, flags));

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

void LineRenderer::realize()
{
    m_program = gl_create_program_from_resource("/org/dune3d/dune3d/canvas/shaders/line-vertex.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/line-fragment.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/line-geometry.glsl");
    m_vao = create_vao(m_program, m_vbo);

    realize_base();
    GET_LOC(this, screen_scale);
    GET_LOC(this, screen);
    GET_LOC(this, line_width);
}

void LineRenderer::push()
{
    m_ca.m_n_lines = m_ca.m_lines.size();
    m_ca.m_n_lines_selection_invisible = m_ca.m_lines_selection_invisible.size();
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Canvas::LineVertex) * (m_ca.m_n_lines + m_ca.m_n_lines_selection_invisible),
                 nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Canvas::LineVertex) * m_ca.m_n_lines, m_ca.m_lines.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(Canvas::LineVertex) * m_ca.m_n_lines,
                    sizeof(Canvas::LineVertex) * m_ca.m_n_lines_selection_invisible,
                    m_ca.m_lines_selection_invisible.data());
}

void LineRenderer::render()
{
    if (!m_ca.m_n_lines && !m_ca.m_n_lines_selection_invisible)
        return;
    glUseProgram(m_program);
    glBindVertexArray(m_vao);

    {
        const auto m = std::min(m_ca.m_width, m_ca.m_height);
        glUniform1f(m_screen_scale_loc, 1e3 / m);
    }
    glUniformMatrix3fv(m_screen_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_screenmat));
    load_uniforms();

    glUniform1f(m_line_width_loc, m_ca.m_appearance.line_width * m_ca.m_scale_factor);

    glDrawArrays(GL_POINTS, 0, m_ca.m_n_lines);
    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawArrays(GL_POINTS, m_ca.m_n_lines, m_ca.m_n_lines_selection_invisible);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

size_t LineRenderer::get_vertex_count() const
{
    return m_ca.m_n_lines;
}

} // namespace dune3d
