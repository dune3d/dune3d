#pragma once
#include <glm/glm.hpp>
#include <epoxy/gl.h>
#include <gtkmm.h>

namespace dune3d {

class Canvas;

class BoxSelection {
public:
    void realize();
    void render();

    void set_active(bool active);
    void set_box(glm::vec2 a, glm::vec2 b);

    BoxSelection(Canvas &c) : m_ca(c)
    {
    }

private:
    Canvas &m_ca;

    GLuint m_program;
    GLuint m_vao;
    GLuint m_vbo;

    GLuint m_screenmat_loc;
    GLuint m_a_loc;
    GLuint m_b_loc;
    GLuint m_fill_loc;
    GLuint m_color_loc;

    glm::vec2 m_sel_a;
    glm::vec2 m_sel_b;
    bool m_active = false;

    void update();
};
} // namespace dune3d
