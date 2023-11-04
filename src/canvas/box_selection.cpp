#include "box_selection.hpp"
#include "canvas.hpp"
#include "gl_util.hpp"
#include "color_palette.hpp"
#include <glm/gtc/type_ptr.hpp>


namespace dune3d {

static GLuint create_vao(GLuint program)
{
    GLuint vao;

    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &vao);

    return vao;
}

void BoxSelection::realize()
{
    m_program = gl_create_program_from_resource(
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-vertex.glsl",
            "/org/dune3d/dune3d/canvas/shaders/"
            "selection-fragment.glsl",
            nullptr);
    m_vao = create_vao(m_program);

    GET_LOC(this, screenmat);
    GET_LOC(this, a);
    GET_LOC(this, b);
    GET_LOC(this, fill);
    GET_LOC(this, color);
}

void BoxSelection::set_active(bool active)
{
    m_active = active;
    m_ca.queue_draw();
}

void BoxSelection::set_box(glm::vec2 a, glm::dvec2 b)
{
    m_sel_a = a;
    m_sel_b = b;
    m_ca.queue_draw();
}

void BoxSelection::render()
{
    if (!m_active)
        return;
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glUniformMatrix3fv(m_screenmat_loc, 1, GL_FALSE, glm::value_ptr(m_ca.m_screenmat));
    glUniform2f(m_a_loc, m_sel_a.x, m_sel_a.y);
    glUniform2f(m_b_loc, m_sel_b.x, m_sel_b.y);
    glUniform1i(m_fill_loc, 0);
    gl_color_to_uniform_3f(m_color_loc, m_ca.m_appearance.get_color(ColorP::SELECTION_BOX));

    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace dune3d
