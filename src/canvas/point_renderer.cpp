#include "point_renderer.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace dune3d {
PointRenderer::PointRenderer(Canvas &c) : BaseRenderer(c, Canvas::VertexType::POINT)
{
}

GLuint PointRenderer::create_vao(GLuint program, GLuint &vbo_out)
{
    GLuint position_index = glGetAttribLocation(program, "position");
    GLuint flags_index = glGetAttribLocation(program, "flags");
    GLuint vao, buffer;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    Canvas::PointVertex vertices[] = {//   Position
                                      {0, 0, 0},
                                      {0, 0, 10},
                                      {0, 10, 0},
                                      {10, 10, 10}};
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::PointVertex), 0);
    glEnableVertexAttribArray(flags_index);
    glVertexAttribIPointer(flags_index, 1, GL_UNSIGNED_INT, sizeof(Canvas::PointVertex),
                           (void *)offsetof(Canvas::PointVertex, flags));

    /* enable and set the color attribute */
    /* reset the state; we will re-enable the VAO when needed */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
    vbo_out = buffer;

    return vao;
}

void PointRenderer::realize()
{
    m_program = gl_create_program_from_resource("/org/dune3d/dune3d/canvas/shaders/point-vertex.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/point-fragment.glsl", nullptr);
    m_vao = create_vao(m_program, m_vbo);
    realize_base();

    GET_LOC(this, z_offset);
}

void PointRenderer::push()
{
    m_ca.m_n_points = m_ca.m_points.size();
    m_ca.m_n_points_selection_invisible = m_ca.m_points_selection_invisible.size();
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Canvas::PointVertex) * (m_ca.m_n_points + m_ca.m_n_points_selection_invisible),
                 nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Canvas::PointVertex) * m_ca.m_n_points, m_ca.m_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(Canvas::PointVertex) * m_ca.m_n_points,
                    sizeof(Canvas::PointVertex) * m_ca.m_n_points_selection_invisible,
                    m_ca.m_points_selection_invisible.data());
}

void PointRenderer::render()
{
    if (!m_ca.m_n_points && !m_ca.m_n_points_selection_invisible)
        return;
    glUseProgram(m_program);
    glBindVertexArray(m_vao);

    load_uniforms();

    glUniform1f(m_z_offset_loc, 0);

    glPointSize(10 * m_ca.m_scale_factor);
    glDrawArrays(GL_POINTS, 0, m_ca.m_n_points);
    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawArrays(GL_POINTS, m_ca.m_n_points, m_ca.m_n_points_selection_invisible);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

size_t PointRenderer::get_vertex_count() const
{
    return m_ca.m_n_points;
}

} // namespace dune3d
